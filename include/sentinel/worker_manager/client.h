//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef SENTINEL_WORKER_MANAGER_CLIENT_H
#define SENTINEL_WORKER_MANAGER_CLIENT_H

#include <basket.h>
#include <memory>

namespace sentinel::worker_manager {

class Client {
private:
    std::shared_ptr<RPC> server_rpc;
public:
    Client();
    void Init();
    void AssignTask(int server_index, int task_id);
    void TerminateWorkerManager(int server_index);
    void FinalizeWorkerManager(int server_index);
    void Finalize();
};

};

#endif //SENTINEL_WORKER_MANAGER_CLIENT_H
