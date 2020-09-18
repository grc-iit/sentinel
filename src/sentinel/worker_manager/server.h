//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef SENTINEL_WORKER_MANAGER_SERVER_H
#define SENTINEL_WORKER_MANAGER_SERVER_H

#include <string>
#include "thread_pool.h"
#include "queue.h"
#include <basket.h>
#include <sentinel/common/debug.h>

namespace sentinel::worker_manager {

struct TaskID {
    uint32_t job_id_ = 0;
    uint32_t task_id_ = 0;

    TaskID() = default;
    TaskID(uint32_t job_id, uint32_t task_id): job_id_(job_id), task_id_(task_id) {}
};

class Worker {
private:
    sentinel::Queue<TaskID> queue_;
public:
    Worker();
    TaskID GetTask();
    void GetAndExecuteTask();
    void ExecuteTask(TaskID task_id);
    void Run(std::future<void> loop_cond);
    void Enqueue(TaskID task_id);
    int GetQueueDepth();
};

class Server {
private:
    sentinel::ThreadPool<Worker> pool_;
    std::shared_ptr<RPC> client_rpc_;
    int num_tasks_assigned_ = 0, min_tasks_assigned_update_ = 512;
    common::debug::Timer epoch_timer_;
    uint32_t epoch_msec_ = 1000;
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
    bool AssignTask(uint32_t job_id, uint32_t task_id);
    bool FinalizeWorkerManager();
};

};

#endif //SENTINEL_WORKER_MANAGER_SERVER_H
