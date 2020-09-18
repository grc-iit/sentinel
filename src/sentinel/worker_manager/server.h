//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef SENTINEL_WORKER_MANAGER_SERVER_H
#define SENTINEL_WORKER_MANAGER_SERVER_H

#include <string>
#include "thread_pool.h"
#include <basket.h>
#include <sentinel/common/debug.h>

namespace sentinel::worker_manager {

class Worker {
private:
    std::list<int> queue_;
private:
    int GetTask();
    void ExecuteTask(int task_id);
public:
    Worker();
    void Run(std::future<void> loop_cond);
    void Enqueue(int task_id);
    int GetQueueDepth();
};

class Server {
private:
    sentinel::ThreadPool<Worker> pool_;
    std::shared_ptr<RPC> client_rpc_;
    int num_tasks_assigned_ = 0, min_tasks_assigned_update_ = 512;
    common::debug::Timer epoch_timer_;
    uint32_t epoch_usec_ = 1000;
    int rank_ = 0;
private:
    bool ReadyToUpdateJobManager();
    bool UpdateJobManager();
    std::shared_ptr<sentinel::worker_manager::Worker> FindMinimumQueue();
    int GetNumTasksQueued(void);
public:
    Server();
    void Init();
    void Run(std::future<void> loop_cond);
    bool AssignTask(uint32_t task_id);
    void KillWorkerManager();
    bool FinalizeWorkerManager();
};

};

#endif //SENTINEL_WORKER_MANAGER_SERVER_H
