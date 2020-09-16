//
// Created by mani on 9/14/2020.
//

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
    job = classLoader.LoadJob(jobId);

    ResourceAllocation defaultResourceAllocation;
    /*
     * Generate default ResourceAllocation
     */

    SpawnTaskManagers(defaultResourceAllocation);

    /*
     * Something has to happen here with the dag
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
    return SpawnTaskManagers(resourceAllocation);
}

bool sentinel::job_manager::Server::SpawnTaskManagers(ResourceAllocation &resourceAllocation) {
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "hostfile",
                 ""); //TODO: path to taskManager hosts (https://www.open-mpi.org/doc/current/man3/MPI_Comm_spawn.3.php)

//    MPI_Comm_spawn(taskManagerExecutable.c_str(), MPI_ARGV_NULL, clusterConfig.taskManagerNodes.size(),
//                   info, 0, MPI_COMM_SELF, &taskManagerComm,
//                   MPI_ERRCODES_IGNORE);
    return true;
}

workmanager_id sentinel::job_manager::Server::FindMinLoad() {
    auto min = *min_element(loadMap.begin(), loadMap.end(),[](const auto &l, const auto &r) { return l.second.num_tasks_queued_ < r.second.num_tasks_queued_; });
    return min.first;
}
