//
// Created by lukemartinlogan on 9/16/20.
//

#include <sentinel/worker_manager/client.h>

sentinel::worker_manager::Client::Client():server_rpc() {
    SENTINEL_CONF->ConfigureWorkermanagerClient();
    Init();
}

void sentinel::worker_manager::Client::Init() {
    this->server_rpc = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
}

bool sentinel::worker_manager::Client::AssignTask(int server_index, int task_id) {
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>(server_index, "AssignTask", task_id).as<bool>();
    return check;
}

void sentinel::worker_manager::Client::KillWorkerManager(int server_index) {
    server_rpc->call<RPCLIB_MSGPACK::object_handle>(server_index, "KillWorkerManager");
}

bool sentinel::worker_manager::Client::FinalizeWorkerManager(int server_index) {
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>(server_index, "FinalizeWorkerManager").as<bool>();
    return check;
}

void sentinel::worker_manager::Client::Finalize() {
}

