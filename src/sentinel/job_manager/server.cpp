#include <sentinel/job_manager/Server.h>


void sentinel::job_manager::Server::Run(std::future<void> futureObj,common::Daemon<Server> * obj) {
    daemon = obj;
    RunInternal(std::move(futureObj));
}

void sentinel::job_manager::Server::RunInternal(std::future<void> futureObj) {

    while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){
        usleep(10000);
    }
}

bool sentinel::job_manager::Server::SubmitJob(uint32_t jobId){
    auto classLoader = ClassLoader();
    std::shared_ptr<Job<Event>> job = classLoader.LoadClass<Job<Event>>(jobId);
    jobs.insert(std::make_pair(jobId, job));

    ResourceAllocation defaultResourceAllocation = SENTINEL_CONF->DEFAULT_RESOURCE_ALLOCATION;
    defaultResourceAllocation.job_id_ = jobId;

    used_resources.insert(std::make_pair(jobId, std::vector<workmanager_id>()));
    SpawnWorkerManagers(defaultResourceAllocation);

    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
    workmanager_id workermanager = used_resources.at(jobId).at(0);

    auto collector = job->GetTask();
    workermanager_client->AssignTask(workermanager, jobId, collector->id_);
    //Lets ensure that load map is not empty
    WorkerManagerStats wms = WorkerManagerStats();
    UpdateWorkerManagerStats(workermanager, wms);
}

bool sentinel::job_manager::Server::TerminateJob(uint32_t jobId){
    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();

    auto possible_job = used_resources.find(jobId);
    if (possible_job == used_resources.end()) return false;

    auto workermanagers_used = used_resources.at(jobId);

    for(auto&& node: workermanagers_used){
        mtx_loadmap.lock();
        WorkerManagerStats reverse_lookup = loadMap.at(node);
        loadMap.erase(node);
        reversed_loadMap.erase(reverse_lookup);
        mtx_loadmap.unlock();

        reversed_used_resources.erase(node);
        available_workermanagers.insert(std::make_pair(node, SENTINEL_CONF->WORKERMANAGER_LISTS[node]));

        workermanager_client->FinalizeWorkerManager(node);
    }
    used_resources.erase(jobId);
}

bool sentinel::job_manager::Server::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats){
    mtx_loadmap.lock();
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()){
        loadMap.insert(std::pair<workmanager_id, WorkerManagerStats>(workerManagerId, stats));
    }
    else {
        possible_load->second = stats;
        reversed_loadMap.erase(stats);
    }
    reversed_loadMap.insert(std::pair<WorkerManagerStats, workmanager_id>(stats, workerManagerId));
    mtx_loadmap.unlock();
    return true;
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::Server::GetWorkerManagerStats(uint32_t workerManagerId){
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) return std::pair<bool, WorkerManagerStats>(false, WorkerManagerStats());
    return std::pair<bool, WorkerManagerStats>(true, possible_load->second);
}

std::vector<std::tuple<uint32_t, uint16_t, task_id>> sentinel::job_manager::Server::GetNextNode(uint32_t job_id, uint32_t currentTaskId, Event event){

    auto newTasks = jobs.at(currentTaskId)->GetTask(currentTaskId)->links;
    auto next_tasks = std::vector<std::tuple<uint32_t, uint16_t, task_id>>();
    for(auto task:newTasks){

        switch(task->type_){
            case TaskType::SOURCE:{
                workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
                next_tasks.push_back(std::tuple<uint32_t,uint16_t, task_id>(newWorkermanager, 0, task->id_));
            }
            case TaskType::KEYBY:{
                KeyByTask<Event>* task_= (KeyByTask<Event>*)((void*)&task);
                auto iter = used_resources.find(job_id);
                if(iter != used_resources.end()){
                    size_t total_workers = iter->second.size();
                    std::hash<size_t> worker_thread_hash;
                    for(auto task:task_->links){
                        auto child_event=event;
                        child_event.id_ += std::to_string(task->id_)+std::to_string(task->job_id_);
                        auto hash = task_->Execute(event);
                        auto worker_index = hash % total_workers;
                        uint16_t worker_thread_id = worker_thread_hash(hash) % SENTINEL_CONF->WORKERTHREAD_COUNT;
                        next_tasks.push_back( std::tuple<uint32_t,uint16_t, task_id>(worker_index,worker_thread_id , task->id_));
                    }
                }else{
                    workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
                    next_tasks.push_back( std::tuple<uint32_t,uint16_t, task_id>(newWorkermanager, 0, task->id_));
                }
            }
            case TaskType::SINK:{
                workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
                next_tasks.push_back( std::tuple<uint32_t,uint16_t, task_id>(newWorkermanager, 0, task->id_));
            }
        }
    }
}

bool sentinel::job_manager::Server::ChangeResourceAllocation(ResourceAllocation &resourceAllocation){
    if( resourceAllocation.num_nodes_ > 0) return SpawnWorkerManagers(resourceAllocation);
    else if( resourceAllocation.num_nodes_ < 0) return TerminateWorkerManagers(resourceAllocation);
    return true;
}

bool sentinel::job_manager::Server::SpawnWorkerManagers(ResourceAllocation &resourceAllocation) {
    char* cmd = SENTINEL_CONF->WORKERMANAGER_EXECUTABLE.data();

    char * mpi_argv[2];
    mpi_argv[0] = SENTINEL_CONF->CONFIGURATION_FILE.data();
    mpi_argv[1] = (char *)0;

    MPI_Info info;
    MPI_Info_create(&info);
    std::string hosts = "localhost";
    for(int i=0; i < resourceAllocation.num_nodes_; i++){
        mtx_allocate.lock();
        auto allocated_workermanager = available_workermanagers.begin();
        available_workermanagers.erase(available_workermanagers.begin());
        mtx_allocate.unlock();

        used_resources.at(resourceAllocation.job_id_).emplace_back(allocated_workermanager->first);
        reversed_used_resources.insert(std::make_pair(allocated_workermanager->first, resourceAllocation.job_id_));
        hosts += "," + allocated_workermanager->second.string();
    }
    MPI_Info_set(info,"host",hosts.data());

    MPI_Comm workerManagerComm;

    int errcodes[resourceAllocation.num_nodes_];

    MPI_Comm_spawn(cmd, mpi_argv, resourceAllocation.num_nodes_, info, 0, MPI_COMM_WORLD, &workerManagerComm, errcodes );

    for(int i=0; i < resourceAllocation.num_nodes_; i++){
        if( errcodes[i] != MPI_SUCCESS) throw ErrorException(SPAWN_WORKERMANAGER_FAILED);;
    }
    return true;
}

bool sentinel::job_manager::Server::TerminateWorkerManagers(ResourceAllocation &resourceAllocation){
    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
    for(int i=resourceAllocation.num_nodes_; i < 0; i++){
        mtx_loadmap.lock();
        workmanager_id workermanager_killed = reversed_loadMap.begin()->second;
        reversed_loadMap.erase(reversed_loadMap.begin());
        loadMap.erase(workermanager_killed);
        mtx_loadmap.unlock();

        available_workermanagers.insert(std::make_pair(workermanager_killed, SENTINEL_CONF->WORKERMANAGER_LISTS[workermanager_killed]));
        auto list_workmanagers = used_resources.at(resourceAllocation.job_id_);
        list_workmanagers.erase(std::remove(list_workmanagers.begin(), list_workmanagers.end(), workermanager_killed), list_workmanagers.end());
        reversed_used_resources.erase(workermanager_killed);

        if(!workermanager_client->FinalizeWorkerManager(workermanager_killed)) throw ErrorException(TERMINATE_WORKERMANAGER_FAILED);;
    }
    return true;
}

