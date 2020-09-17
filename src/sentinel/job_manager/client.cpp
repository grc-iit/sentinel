//
// Created by mani on 9/14/2020.
//

#include <sentinel/job_manager/client.h>

sentinel::job_manager::client::client(){
    SENTINEL_CONF->ConfigureJobmanagerClient();
    auto basket=BASKET_CONF;
    rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
}

bool sentinel::job_manager::client::SubmitJob(uint32_t jobId) {
    int server = 0;
    auto num_servers = rpc->call<RPCLIB_MSGPACK::object_handle>(server, "SubmitJob", jobId) .as<bool>();
}

bool sentinel::job_manager::client::UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats) {
    int server = 0;
    auto ret = rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetWorkerManagerStats", workerManagerId,stats).as<bool>();
}

std::pair<bool, WorkerManagerStats> sentinel::job_manager::client::GetWorkerManagerStats(uint32_t workerManagerId) {
    int server = 0;
    auto ret = rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetWorkerManagerStats", workerManagerId).as<std::pair<bool, WorkerManagerStats>>();
}

std::pair<uint32_t, uint32_t> sentinel::job_manager::client::GetNextNode(uint32_t currentTaskId) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "GetNextNode", currentTaskId).as<std::pair<uint32_t, uint32_t>>();
}

bool sentinel::job_manager::client::ChangeResourceAllocation(ResourceAllocation &resourceAllocation) {
    int server = 0;
    return rpc->call<RPCLIB_MSGPACK::object_handle>(server, "ChangeResourceAllocation", resourceAllocation).as<bool>();
}
