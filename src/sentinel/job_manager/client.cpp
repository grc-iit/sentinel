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
    int server = 0;
    auto num_servers = rpc->call<RPCLIB_MSGPACK::object_handle>(server, "SubmitJob", jobId,num_sources) .as<bool>();
}

bool sentinel::job_manager::client::TerminateJob(uint32_t jobId) {
    int server = 0;
    auto num_servers = rpc->call<RPCLIB_MSGPACK::object_handle>(server, "TerminateJob", jobId) .as<bool>();
}

bool sentinel::job_manager::client::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "UpdateWorkerManagerStats", workerManagerId, stats).as<bool>();
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::client::GetWorkerManagerStats(uint32_t workerManagerId) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetWorkerManagerStats", workerManagerId).as<std::pair<bool, WorkerManagerStats>>();
}

std::vector<std::tuple<uint32_t, uint16_t, uint32_t>> sentinel::job_manager::client::GetNextNode(uint32_t job_id, uint32_t currentTaskId, Event event) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetNextNode", job_id, currentTaskId, event).as<std::vector<std::tuple<uint32_t, uint16_t, uint32_t>>>();
}

bool sentinel::job_manager::client::ChangeResourceAllocation(ResourceAllocation &resourceAllocation) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "ChangeResourceAllocation", resourceAllocation).as<bool>();
}
