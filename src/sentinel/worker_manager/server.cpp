//
// Created by lukemartinlogan on 9/16/20.
//

#include "server.h"
#include "thread_pool.h"
#include <common/debug.h>
#include <common/class_loader.h>
#include <basket/common/configuration_manager.h>
#include <sentinel/common/configuration_manager.h>
#include <sentinel/job_manager/client.h>
#include <basket/common/singleton.h>
#include <basket/communication/rpc_factory.h>
#include <rpc/client.h>
#include <basket.h>
#include <thread>
#include <limits>

/*
 * SERVER
 * */

sentinel::worker_manager::Server::Server() {
    AUTO_TRACER("sentinel::worker_manager::Server::Server");
    SENTINEL_CONF->ConfigureWorkermanagerServer();
    Init();
    pool_.Init(4); //TODO: Configuration Manager
    epoch_timer_.startTime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
}

void sentinel::worker_manager::Server::Init() {
    AUTO_TRACER("sentinel::worker_manager::Server::Init");
    client_rpc_ = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

    std::function<bool(uint32_t, uint32_t)> functionAssignTask(std::bind(
            &sentinel::worker_manager::Server::AssignTask,
            this, std::placeholders::_1, std::placeholders::_2));
    client_rpc_->bind("AssignTask", functionAssignTask);

    std::function<bool(void)>  functionFinalizeWorkerManager(std::bind(&sentinel::worker_manager::Server::FinalizeWorkerManager, this));
    client_rpc_->bind("FinalizeWorkerManager", functionFinalizeWorkerManager);
}

void sentinel::worker_manager::Server::Run(std::future<void> loop_cond) {
    AUTO_TRACER("sentinel::worker_manager::Server::Run");
    while(loop_cond.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {}
}

std::shared_ptr<sentinel::worker_manager::Worker> sentinel::worker_manager::Server::FindMinimumQueue(void) {
    /**
     * TODO: Dont ping the queues all the time. Maintain a reverse ordered_map datastructure and do a head on that
     * Keep this operation O(1)
     */

    AUTO_TRACER("sentinel::worker_manager::Server::FindMinimumQueue");
    std::shared_ptr<Worker> worker;
    int min_queue_size = std::numeric_limits<int>::max();

    for(Thread<Worker> &thread : pool_) {
        if(thread.IsActive()) {
            std::shared_ptr<Worker> temp = std::move(thread.GetObj());
            if(temp->GetQueueDepth() < min_queue_size) {
                min_queue_size = temp->GetQueueDepth();
                worker = std::move(temp);
            }
        }
    }

    return std::move(worker);
}

int sentinel::worker_manager::Server::GetNumTasksQueued(void) {
    /**
     * TODO: Dont ping the queues all the time.
     * Approach 1: Maintain a counter locally an atomic_int which can be updated
     * Approach 2: also try keeping a array of size max threads for each thread this will avoid locking
     * Check which is faster through quick benchmark. (my intuition is both should be fine)
     */

    AUTO_TRACER("sentinel::worker_manager::Server::GetNumTasksQueued");
    int num_tasks_queued = 0;

    for(Thread<Worker> &thread : pool_) {
        if(thread.IsActive()) {
            std::shared_ptr<Worker> temp = std::move(thread.GetObj());
            num_tasks_queued += temp->GetQueueDepth();
        }
    }

    return num_tasks_queued;
}

bool sentinel::worker_manager::Server::ReadyToUpdateJobManager() {
    AUTO_TRACER("sentinel::worker_manager::Server::ReadyToUpdateJobManager");
    return (num_tasks_assigned_ > min_tasks_assigned_update_); //TODO: Check clock
}

bool sentinel::worker_manager::Server::UpdateJobManager() {
    AUTO_TRACER("sentinel::worker_manager::Server::UpdateJobManager");
    double time_ms = epoch_timer_.endTime();
    int num_tasks_queued = GetNumTasksQueued();
    WorkerManagerStats wms(time_ms, num_tasks_assigned_, num_tasks_queued);
    auto jm = basket::Singleton<sentinel::job_manager::client>::GetInstance();
    bool check = jm->UpdateWorkerManagerStats(rank_, wms);
    epoch_timer_.startTime();
    return check;
}

bool sentinel::worker_manager::Server::AssignTask(uint32_t job_id, uint32_t task_id) {
    AUTO_TRACER("sentinel::worker_manager::Server::AssignTask", task_id);

    //Spawn a new thread if there are any available in the pool
    if(pool_.Size() < pool_.MaxSize()) {
        pool_.Assign();
    }

    //Enqueue work in existing thread
    std::shared_ptr<Worker> thread = FindMinimumQueue();
    thread->Enqueue(TaskID(job_id, task_id));
    ++num_tasks_assigned_;

    //Update Job Manager
    if(ReadyToUpdateJobManager()) {
        UpdateJobManager();
    }
    return true;
}

bool sentinel::worker_manager::Server::FinalizeWorkerManager() {
    AUTO_TRACER("sentinel::worker_manager::Server::FinalizeWorkerManager");
    pool_.StopAll();
    return true;
}

/*
 * THREAD
 * */

sentinel::worker_manager::Worker::Worker() {
    AUTO_TRACER("sentinel::worker_manager::Worker::Worker");
}

sentinel::worker_manager::TaskID sentinel::worker_manager::Worker::GetTask() {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetTask");
    TaskID id;
    queue_.Pop(id);
    return std::move(id);
}

void sentinel::worker_manager::Worker::ExecuteTask(TaskID id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::ExecuteTask", task_id);
    std::shared_ptr<Job> job = ClassLoader().LoadClass<Job>(id.job_id_);
    std::shared_ptr<Task> task = job->GetTask(id.task_id_);
    task->Execute();
}

void sentinel::worker_manager::Worker::GetAndExecuteTask() {
    ExecuteTask(GetTask());
}

void sentinel::worker_manager::Worker::Run(std::future<void> loop_cond) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Run", task_id);
    bool kill_if_empty = false;
    while(loop_cond.wait_for(std::chrono::milliseconds(100))==std::future_status::timeout) {
        if(queue_.Size() == 0 && kill_if_empty) {
            return;
        }
        while (queue_.Size() > 0) {
            GetAndExecuteTask();
        }
        kill_if_empty = true;
    }
}

void sentinel::worker_manager::Worker::Enqueue(TaskID id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Enqueue", task_id);
    queue_.Push(id);
}

int sentinel::worker_manager::Worker::GetQueueDepth() {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetQueueDepth");
    return queue_.Size();
}