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

bool sentinel::worker_manager::Client::AssignTask(
        int server_index,
        uint32_t worker_tid_min, uint32_t worker_tid_count,
        uint32_t job_id, uint32_t task_id, Event &event) {
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>(
            server_index, "AssignTask",
            worker_tid_min, worker_tid_count,
            job_id, task_id, event).as<bool>();
    return check;
}

bool sentinel::worker_manager::Client::FinalizeWorkerManager(int server_index) {
    auto check = server_rpc->call<RPCLIB_MSGPACK::object_handle>(server_index, "FinalizeWorkerManager").as<bool>();
    return check;
}

void sentinel::worker_manager::Client::Finalize() {
}

