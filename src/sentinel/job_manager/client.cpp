//
// Created by mani on 9/14/2020.
//

#include <sentinel/job_manager/client.h>

sentinel::job_manager::client::client(){
    SENTINEL_CONF->ConfigureJobmanagerClient();
    auto basket=BASKET_CONF;
    rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
}

bool sentinel::job_manager::client::SubmitJob(uint32_t jobId, uint32_t num_sources) {
    AUTO_TRACER("job_manager::client::SubmitJob", jobId,num_sources);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "SubmitJob", jobId,num_sources) .as<bool>();
}

bool sentinel::job_manager::client::TerminateJob(uint32_t jobId) {
    AUTO_TRACER("job_manager::client::TerminateJob", jobId);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "TerminateJob", jobId) .as<bool>();
}

bool sentinel::job_manager::client::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats) {
    AUTO_TRACER("job_manager::client::UpdateWorkerManagerStats", workerManagerId,stats);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "UpdateWorkerManagerStats", workerManagerId, stats).as<bool>();
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::client::GetWorkerManagerStats(uint32_t workerManagerId) {
    AUTO_TRACER("job_manager::client::GetWorkerManagerStats", workerManagerId);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetWorkerManagerStats", workerManagerId).as<std::pair<bool, WorkerManagerStats>>();
}

std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>> sentinel::job_manager::client::GetNextNode(JobId job_id, TaskId currentTaskId, Event &event) {
    AUTO_TRACER("job_manager::client::GetNextNode", job_id, currentTaskId, event);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetNextNode", job_id, currentTaskId, event).as<std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>>>();
}

bool sentinel::job_manager::client::ChangeResourceAllocation(ResourceAllocation &resourceAllocation) {
    AUTO_TRACER("job_manager::client::ChangeResourceAllocation", resourceAllocation);
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "ChangeResourceAllocation", resourceAllocation).as<bool>();
}
