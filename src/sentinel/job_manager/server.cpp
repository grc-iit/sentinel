#include <sentinel/job_manager/Server.h>


void sentinel::job_manager::Server::Run(std::future<void> futureObj,common::Daemon<Server> * obj) {
    daemon = obj;
    RunInternal(std::move(futureObj));
}

void sentinel::job_manager::Server::RunInternal(std::future<void> futureObj) {
//    this->SubmitJob(0,1);
    while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){
        usleep(10000);
    }
}

bool sentinel::job_manager::Server::SubmitJob(uint32_t jobId, uint32_t num_sources){
    auto classLoader = ClassLoader();
    std::shared_ptr<Job<Event>> job = classLoader.LoadClass<Job<Event>>(jobId);
    jobs.insert(std::make_pair(jobId, job));

    ResourceAllocation defaultResourceAllocation = SENTINEL_CONF->DEFAULT_RESOURCE_ALLOCATION;
    defaultResourceAllocation.job_id_ = jobId;

    used_resources.insert(std::make_pair(jobId, std::vector<std::tuple<workmanager_id,uint32_t,uint32_t>>()));
    auto threads = defaultResourceAllocation.num_nodes_ * defaultResourceAllocation.num_threads_per_proc * defaultResourceAllocation.num_threads_per_proc;
    SpawnWorkerManagers(threads, jobId);
    sleep(1);
    auto collector = job->GetTask();
    auto current_worker_index = 0;
    auto workers = used_resources.at(jobId);
    auto current_worker_thread = std::get<1>(workers[0]);
    for(int i=0;i<num_sources;i++){
        auto end_thread = std::get<2>(workers[current_worker_index]);
        workmanager_id workermanager = current_worker_index;
        Event event;
        event.id_ = std::to_string(i);
        workermanager_client->AssignTask(workermanager,0, jobId, collector->id_,event);
        //Lets ensure that load map is not empty
        WorkerManagerStats wms = WorkerManagerStats();
        UpdateWorkerManagerStats(workermanager, wms);
        current_worker_thread++;
        if(current_worker_thread == end_thread){
            current_worker_index++;
            if(workers.size() == current_worker_index && i != num_sources){
                //TODO: throw error.
                break;
            }else current_worker_thread = std::get<1>(workers[current_worker_index]);
        }
    }

}

bool sentinel::job_manager::Server::TerminateJob(uint32_t jobId){
    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();

    auto possible_job = used_resources.find(jobId);
    if (possible_job == used_resources.end()) return false;

    auto workermanagers_used = used_resources.at(jobId);
    for(auto workermanager_used:workermanagers_used){
        auto worker_index = std::get<0>(workermanager_used);
        auto start_thread = std::get<1>(workermanager_used);
        auto end_thread = std::get<2>(workermanager_used);
        /**
         * TODO: fixme for all edge cases.
         */
        if(start_thread == 0 && end_thread == SENTINEL_CONF->WORKERTHREAD_COUNT - 1){
            mtx_loadmap.lock();
            WorkerManagerStats reverse_lookup = loadMap.at(worker_index);
            loadMap.erase(worker_index);
            reversed_loadMap.erase(reverse_lookup);
            available_workermanagers.erase(worker_index);
            available_workermanagers.insert({worker_index, {SENTINEL_CONF->WORKERMANAGER_LISTS[worker_index],SENTINEL_CONF->WORKERTHREAD_COUNT}});
            mtx_loadmap.unlock();
            workermanager_client->FinalizeWorkerManager(worker_index);
        }
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
//                workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
//                next_tasks.push_back(std::tuple<uint32_t,uint16_t, task_id>(newWorkermanager, 0, task->id_));
                break;
            }
            case TaskType::KEYBY:{
                auto iter = used_resources.find(job_id);
                size_t total_workers = iter->second.size();
                std::hash<size_t> worker_thread_hash;
                for(auto childtask:task->links){
                    auto child_event=event;
                    child_event.id_ += std::to_string(childtask->id_)+std::to_string(childtask->job_id_);
                    auto hash_event = task->Execute(event);
                    auto hash = atoi(hash_event.id_.c_str());
                    auto worker_index = hash % total_workers;
                    uint16_t worker_thread_id = worker_thread_hash(hash) % SENTINEL_CONF->WORKERTHREAD_COUNT;
                    next_tasks.push_back( std::tuple<uint32_t,uint16_t, task_id>(worker_index,worker_thread_id , childtask->id_));
                }
                break;
            }
            case TaskType::SINK:{
                workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
                next_tasks.push_back(std::tuple<uint32_t,uint16_t, task_id>(newWorkermanager, 0, task->id_));
                break;
            }
        }
    }
    return next_tasks;
}

bool sentinel::job_manager::Server::ChangeResourceAllocation(ResourceAllocation &resourceAllocation){
    auto threads = resourceAllocation.num_nodes_ * resourceAllocation.num_threads_per_proc * resourceAllocation.num_threads_per_proc;
    if( resourceAllocation.num_nodes_ > 0) return SpawnWorkerManagers(threads,resourceAllocation.job_id_);
    else if( resourceAllocation.num_nodes_ < 0) return TerminateWorkerManagers(resourceAllocation);
    return true;
}

bool sentinel::job_manager::Server::SpawnWorkerManagers(uint32_t required_threads, job_id job_id_) {
    char* cmd = SENTINEL_CONF->WORKERMANAGER_EXECUTABLE.data();

    char * mpi_argv[2];
    mpi_argv[0] = SENTINEL_CONF->CONFIGURATION_FILE.data();
    mpi_argv[1] = (char *)0;

    MPI_Info info;
    MPI_Info_create(&info);
    std::string hosts = "localhost";
    auto left_threads = required_threads;
    auto available_worker_iter = available_workermanagers.begin();
    auto new_worker_spawn = std::vector<uint32_t>();
    while(left_threads > 0){
        auto worker_index = available_worker_iter->first;
        auto host = available_worker_iter->second.first;
        auto start_thread = SENTINEL_CONF->WORKERTHREAD_COUNT - available_worker_iter->second.second;
        if(start_thread == 0){
            new_worker_spawn.push_back(worker_index);
            hosts += "," + host.string();
        }
        auto can_use_threads = available_worker_iter->second.second < left_threads ? available_worker_iter->second.second : left_threads;

        auto end_thread = start_thread + can_use_threads - 1;
        auto left_thread_in_worker = available_worker_iter->second.second - can_use_threads;
        available_workermanagers.erase(available_worker_iter);
        if(left_thread_in_worker > 0){
            available_workermanagers.insert({worker_index,{host,left_thread_in_worker}});
        }
        used_resources.at(job_id_).emplace_back(std::tuple<workmanager_id,uint32_t,uint32_t>(worker_index,start_thread,end_thread));
        left_threads -= can_use_threads;

    }
    if(new_worker_spawn.size() > 0){
        MPI_Info_set(info,"host", hosts.data());
        MPI_Comm workerManagerComm;
        int errcodes[new_worker_spawn.size()];

        MPI_Comm_spawn(cmd, mpi_argv, new_worker_spawn.size(), info, 0, MPI_COMM_WORLD, &workerManagerComm, errcodes );

        for(int i=0; i < new_worker_spawn.size(); i++){
            if( errcodes[i] != MPI_SUCCESS) throw ErrorException(SPAWN_WORKERMANAGER_FAILED);;
        }
    }

    return true;
}

bool sentinel::job_manager::Server::TerminateWorkerManagers(ResourceAllocation &resourceAllocation){
//    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
//    for(int i=resourceAllocation.num_nodes_; i < 0; i++){
//        mtx_loadmap.lock();
//        workmanager_id workermanager_killed = reversed_loadMap.begin()->second;
//        reversed_loadMap.erase(reversed_loadMap.begin());
//        loadMap.erase(workermanager_killed);
//        mtx_loadmap.unlock();
//
//
//
//        available_workermanagers.insert(std::make_pair(workermanager_killed, SENTINEL_CONF->WORKERMANAGER_LISTS[workermanager_killed]));
//        auto list_workmanagers = used_resources.at(resourceAllocation.job_id_);
//        list_workmanagers.erase(std::remove(list_workmanagers.begin(), list_workmanagers.end(), workermanager_killed), list_workmanagers.end());
//
//        if(!workermanager_client->FinalizeWorkerManager(workermanager_killed)) throw ErrorException(TERMINATE_WORKERMANAGER_FAILED);;
//    }
    return true;
}

