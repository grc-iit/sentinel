//
// Created by lukemartinlogan on 9/16/20.
//

#include <sentinel/worker_manager/client.h>

sentinel::worker_manager::Client::Client():server_rpc() {
    AUTO_TRACER("worker_manager::Client::Client");
    SENTINEL_CONF->ConfigureWorkermanagerClient();
    Init();
}

void sentinel::worker_manager::Client::Init() {
    AUTO_TRACER("worker_manager::Client::Init");
    this->server_rpc = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
}

ThreadId sentinel::worker_manager::Client::AssignTask(int server_index, std::set<ThreadId> threads, uint32_t job_id, uint32_t task_id, Event &event) {
    AUTO_TRACER("worker_manager::Client::AssignTask",server_index,threads,job_id,task_id,event);
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>( server_index, "AssignTask", threads, job_id, task_id, event).as<ThreadId>();
    return check;
}

bool sentinel::worker_manager::Client::FinalizeWorkerManager(int server_index) {
    AUTO_TRACER("worker_manager::Client::FinalizeWorkerManager",server_index);
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>(server_index, "FinalizeWorkerManager").as<bool>();
    return check;
}

