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
    job = classLoader.LoadClass<Job>(jobId);
    ResourceAllocation defaultResourceAllocation = SENTINEL_CONF->DEFAULT_RESOURCE_ALLOCATION;
    /*
     * Generate default ResourceAllocation
     */
    SpawnWorkerManagers(defaultResourceAllocation);
    /*
     * Something has to happen here with the dag
     */
    /**
     * TODO: allocate the first node source to the allocated worker manager.
     *
     */

}

bool sentinel::job_manager::Server::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats){

    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) loadMap.insert(std::pair<workmanager_id, WorkerManagerStats>(workerManagerId, stats));
    else possible_load->second = stats;
    return true;
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::Server::GetWorkerManagerStats(uint32_t workerManagerId){
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) return std::pair<bool, WorkerManagerStats>(false, WorkerManagerStats());
    return std::pair<bool, WorkerManagerStats>(true, possible_load->second);
}

std::pair<workmanager_id, task_id> sentinel::job_manager::Server::GetNextNode(uint32_t currentTaskId){
    /**
     * use ordered_map to do O(1) traversal to get the least node load.
     */
    auto possible_destination = destinationMap.find(currentTaskId);
    workmanager_id currentWorkermanagerId = possible_destination->second.first;
    //If we dont have a current destination, or the destination is over a certain fullness
    if (possible_destination == destinationMap.end() ||
        loadMap.at(currentWorkermanagerId).num_tasks_queued_ > SENTINEL_CONF->MAX_LOAD) {
        //find a new destination
        workmanager_id newWorkermanager = FindMinLoad();
        task_id newTask;
        if(possible_destination == destinationMap.end()){
            //What is the id of the next task
            newTask = job->GetNextTaskId(currentTaskId);
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
    return SpawnWorkerManagers(resourceAllocation);
}

bool sentinel::job_manager::Server::SpawnWorkerManagers(ResourceAllocation &resourceAllocation) {
    MPI_Comm taskManagerComm;
    MPI_Info info;
    MPI_Info_create(&info);
    /**
     * TODO: Maintain current node , proc within node and thread index within Server class
     * - On allocate u calculate the nodes based on list and given allocation size.
     * - Build a list of hosts and then call the mpi spawn.
     */


    MPI_Info_set(info, "hostfile", SENTINEL_CONF->WORKERMANAGER_DINAMIC_HOSTFILE.c_str());

    char** spawn_argv = static_cast<char **>(malloc(1 * sizeof(char *)));
    spawn_argv[0] = (char*) SENTINEL_CONF->CONFIGURATION_FILE.c_str();

    // TODO: append new nodes to the hostfile

    MPI_Comm_spawn(SENTINEL_CONF->WORKERMANAGER_EXECUTABLE.c_str(), spawn_argv, resourceAllocation.num_nodes_,
                   info, 0, MPI_COMM_SELF, &taskManagerComm,
                   MPI_ERRCODES_IGNORE);

    //Merge the mpi_comm and maintain it, to be able to talk to them?
    //TODO: is this really necessary, are we doing any mpi comm?

    return true;
}

workmanager_id sentinel::job_manager::Server::FindMinLoad() {
    auto min = *min_element(loadMap.begin(), loadMap.end(),[](const auto &l, const auto &r) { return l.second.num_tasks_queued_ < r.second.num_tasks_queued_; });
    return min.first;
}