#include <sentinel/job_manager/Server.h>


void sentinel::job_manager::Server::Run(std::future<void> futureObj) {
    RunInternal(std::move(futureObj));
}

void sentinel::job_manager::Server::RunInternal(std::future<void> futureObj) {
    while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){
        usleep(10000);
    }
}

bool sentinel::job_manager::Server::SubmitJob(uint32_t jobId){
    auto classLoader = ClassLoader();
    std::shared_ptr<Job> job = classLoader.LoadClass<Job>(jobId);
    jobs.insert(std::make_pair(jobId, job));

    ResourceAllocation defaultResourceAllocation = SENTINEL_CONF->DEFAULT_RESOURCE_ALLOCATION;
    defaultResourceAllocation.jobId = jobId;

    used_resources.insert(std::make_pair(jobId, std::vector<workmanager_id>()));
    SpawnWorkerManagers(defaultResourceAllocation);

    auto workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
    workmanager_id workermanager = used_resources.at(jobId).at(0);

    uint32_t collector_id = job->GetCollectorId();
    workermanager_client->AssignTask(workermanager, collector_id);
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

        if(!workermanager_client->FinalizeWorkerManager(node)) return false;
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

std::pair<workmanager_id, task_id> sentinel::job_manager::Server::GetNextNode(uint32_t workermanagerId, uint32_t currentTaskId){
    auto possible_destination = destinationMap.find(currentTaskId);
    workmanager_id currentWorkermanagerId = possible_destination->second.first;
    //If we dont have a current destination, or the destination is over a certain fullness
    if (possible_destination == destinationMap.end() ||
        loadMap.at(currentWorkermanagerId).num_tasks_queued_ > SENTINEL_CONF->MAX_LOAD) {
        //find a new destination
        workmanager_id newWorkermanager = reversed_loadMap.begin()->second;
        task_id newTask;
        if(possible_destination == destinationMap.end()){
            //What is the id of the next task
            job_id current_job = reversed_used_resources.at(workermanagerId);
            newTask = jobs.at(current_job)->GetNextTaskId(currentTaskId);
            destinationMap.insert(std::pair<task_id, std::pair<workmanager_id, task_id>>(currentTaskId,
                    std::pair<workmanager_id, task_id>(newWorkermanager, newTask)));
        }
        else {
            newTask = possible_destination->second.second;
            possible_destination->second = std::pair<workmanager_id, task_id>(newWorkermanager, newTask);
        }
    }
    return possible_destination->second;
}

bool sentinel::job_manager::Server::ChangeResourceAllocation(ResourceAllocation &resourceAllocation){
    if( resourceAllocation.num_nodes_ > 0) return SpawnWorkerManagers(resourceAllocation);
    else if( resourceAllocation.num_nodes_ < 0) return TerminateWorkerManagers(resourceAllocation);
    return true;
}

bool sentinel::job_manager::Server::SpawnWorkerManagers(ResourceAllocation &resourceAllocation) {
    /**
     * TODO: Maintain current node , proc within node and thread index within Server class
     * - On allocate u calculate the nodes based on list and given allocation size.
     * - Build a list of hosts and then call the mpi spawn.
    */

    MPI_Comm taskManagerComm;
    MPI_Info info[resourceAllocation.num_nodes_];
    char *cmds[resourceAllocation.num_nodes_];
    char **spawn_argv[resourceAllocation.num_nodes_];
    int np[resourceAllocation.num_nodes_];
    int errcodes[resourceAllocation.num_nodes_];
    for(int i=0; i < resourceAllocation.num_nodes_; i++){
        mtx_allocate.lock();
        auto allocated_workermanager = available_workermanagers.begin();
        available_workermanagers.erase(available_workermanagers.begin());
        mtx_allocate.unlock();

        used_resources.at(resourceAllocation.jobId).emplace_back(allocated_workermanager->first);
        reversed_used_resources.insert(std::make_pair(allocated_workermanager->first, resourceAllocation.jobId));

        MPI_Info_create(&info[i]);
        MPI_Info_set(info[i],"host","node1");
        cmds[i] = { SENTINEL_CONF->WORKERMANAGER_EXECUTABLE.data()};
        spawn_argv[i][0] = { SENTINEL_CONF->CONFIGURATION_FILE.data()}; //TODO: this might be one
        np[i] = {1};
    }
    MPI_Comm_spawn_multiple(resourceAllocation.num_nodes_, cmds, spawn_argv, np, info, 0, MPI_COMM_WORLD, &taskManagerComm, errcodes );

    for(int i=0; i < resourceAllocation.num_nodes_; i++){
        if( errcodes[i] != MPI_SUCCESS) return false;
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
        auto list_workmanagers = used_resources.at(resourceAllocation.jobId);
        list_workmanagers.erase(std::remove(list_workmanagers.begin(), list_workmanagers.end(), workermanager_killed), list_workmanagers.end());
        reversed_used_resources.erase(workermanager_killed);

        if(!workermanager_client->FinalizeWorkerManager(workermanager_killed)) return false;
    }
    return true;
}

