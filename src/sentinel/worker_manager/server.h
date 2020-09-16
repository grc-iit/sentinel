//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef SENTINEL_WORKER_MANAGER_SERVER_H
#define SENTINEL_WORKER_MANAGER_SERVER_H

#include <string>
#include "thread_pool.h"
#include <basket.h>

namespace sentinel::worker_manager {

class Worker {
private:
    std::list<int> q_;
private:
    int GetTask();
    void ExecuteTask(int task_id);
public:
    WorkerThread();
    void Run(std::future<void> loop_cond);
    void Enqueue(int task_id);
    int GetQueueDepth();
};

class Server {
private:
    sentinel::ThreadPool pool_;
    std::shared_ptr<RPC> client_rpc_;
    int num_tasks_exec_, min_tasks_exec_update_;
    //TODO: Add clock_t for epoch
private:
    bool ReadyToUpdateJobManager();
    void UpdateJobManager();
    Worker &FindMinimumQueue();
public:
    Server();
    void Init();
    void AssignTask(int task_id);
    void Terminate();
    void Finalize();
};

};

#endif //SENTINEL_WORKER_MANAGER_SERVER_H
