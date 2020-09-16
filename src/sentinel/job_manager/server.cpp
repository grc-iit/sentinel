//
// Created by mani on 9/14/2020.
//

#include <sentinel/job_manager/server.h>

sentinel::job_manager::server::server(){
    SENTINEL_CONF->ConfigureJobManagerServer();
    auto basket=BASKET_CONF;
    rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
    std::function<bool(uint32_t)> functionSubmitJob(std::bind(&sentinel::job_manager::server::SubmitJob, this, std::placeholders::_1));
    std::function<bool(uint32_t,WorkerManagerStats&)> functionUpdateWorkerManagerStats(std::bind(&sentinel::job_manager::server::UpdateWorkerManagerStats, this, std::placeholders::_1, std::placeholders::_2));
    std::function<std::pair<bool, WorkerManagerStats>(uint32_t)> functionGetWorkerManagerStats(std::bind(&sentinel::job_manager::server::GetWorkerManagerStats, this, std::placeholders::_1));
    std::function<std::pair<uint32_t, uint32_t>(uint32_t)> functionGetNextNode(std::bind(&sentinel::job_manager::server::GetNextNode, this, std::placeholders::_1));
    std::function<bool(ResourceAllocation&)> functionChangeResourceAllocation(std::bind(&sentinel::job_manager::server::ChangeResourceAllocation, this, std::placeholders::_1));
    rpc->bind("SubmitJob", functionSubmitJob);
    rpc->bind("UpdateWorkerManagerStats", functionUpdateWorkerManagerStats);
    rpc->bind("GetWorkerManagerStats", functionGetWorkerManagerStats);
    rpc->bind("GetNextNode", functionGetNextNode);
    rpc->bind("ChangeResourceAllocation", functionChangeResourceAllocation);
}

void sentinel::job_manager::server::Run(std::future<void> futureObj) {
    RunInternal(std::move(futureObj));
}

void sentinel::job_manager::server::RunInternal(std::future<void> futureObj) {
    while(futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout){
        usleep(10000);
    }
}

bool sentinel::job_manager::server::SubmitJob(uint32_t jobId){
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

bool sentinel::job_manager::server::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats){
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) loadMap.insert(std::pair<workmanager_id, WorkerManagerStats>(workerManagerId, stats));
    else possible_load->second = stats;
    return true;
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::server::GetWorkerManagerStats(uint32_t workerManagerId){
    auto possible_load = loadMap.find(workerManagerId);
    if (possible_load == loadMap.end()) return std::pair<bool, WorkerManagerStats>(false, WorkerManagerStats());
    return std::pair<bool, WorkerManagerStats>(true, possible_load->second);
}

std::pair<workmanager_id, task_id> sentinel::job_manager::server::GetNextNode(uint32_t currentTaskId){
    auto possible_destination = destinationMap.find(currentTaskId);
    workmanager_id currentWorkermanagerId = possible_destination->second.first;
    //If we dont have a current destination, or the destination is over a certain fullness
    if (possible_destination == destinationMap.end() ||
        loadMap.at(currentWorkermanagerId) > SENTINEL_CONF->MAX_LOAD) {
            //find a new destination
            workmanager_id newWorkermanager = findMinLoad();
            task_id newTask;
            if(possible_destination == destinationMap.end()){
                //What is the id of the next task
                newTask = job.GetNextTaskId(currentTaskId);
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

bool sentinel::job_manager::server::ChangeResourceAllocation(ResourceAllocation &resourceAllocation){
    return SpawnTaskManagers(resourceAllocation);
}

bool sentinel::job_manager::server::SpawnTaskManagers(ResourceAllocation &resourceAllocation) {
    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info, "hostfile",
                 ""); //TODO: path to taskManager hosts (https://www.open-mpi.org/doc/current/man3/MPI_Comm_spawn.3.php)

    MPI_Comm_spawn(taskManagerExecutable.c_str(), MPI_ARGV_NULL, clusterConfig.taskManagerNodes.size(),
                   info, 0, MPI_COMM_SELF, &taskManagerComm,
                   MPI_ERRCODES_IGNORE);
    return true;
}

workmanager_id sentinel::job_manager::server::findMinLoad() {
    auto min = *min_element(loadMap.begin(), loadMap.end(),[](const auto &l, const auto &r) { return l.second < r.second; });
    return min.first;
}
