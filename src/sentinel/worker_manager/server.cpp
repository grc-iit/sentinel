//
// Created by lukemartinlogan on 9/16/20.
//

#include "server.h"
#include "thread_pool.h"
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
    SENTINEL_CONF->ConfigureWorkermanagerServer();
    Init();
    epoch_timer_.startTime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
}

void sentinel::worker_manager::Server::Init() {
    client_rpc_ = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

    std::function<void(uint32_t)> functionAssignTask(std::bind(&sentinel::worker_manager::Server::AssignTask, this, std::placeholders::_1));
    client_rpc_->bind("AssignTask", functionAssignTask);

    std::function<void(void)> functionTerminateWorkerManager(std::bind(&sentinel::worker_manager::Server::Terminate, this));
    client_rpc_->bind("TerminateWorkerManager", functionTerminateWorkerManager);

    std::function<void(void)>  functionFinalizeWorkerManager(std::bind(&sentinel::worker_manager::Server::Finalize, this));
    client_rpc_->bind("FinalizeWorkerManager", functionFinalizeWorkerManager);
}

void sentinel::worker_manager::Server::Run(std::future<void> loop_cond) {
    while(loop_cond.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {}
}

std::shared_ptr<sentinel::worker_manager::Worker> sentinel::worker_manager::Server::FindMinimumQueue(void) {
    /**
     * TODO: Dont ping the queues all the time. Maintain a reverse ordered_map datastructure and do a head on that
     * Keep this operation O(1)
     */
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
    return (num_tasks_assigned_ > min_tasks_assigned_update_); //TODO: Check clock
}

void sentinel::worker_manager::Server::UpdateJobManager() {
    double time_ms = epoch_timer_.endTime();
    int num_tasks_queued = GetNumTasksQueued();
    WorkerManagerStats wms(time_ms, num_tasks_assigned_, num_tasks_queued);
    //TODO: Send worker manager stats to JobManager
    auto jm = basket::Singleton<sentinel::job_manager::client>::GetInstance();
    jm->UpdateWorkerManagerStats(rank_, wms);
    epoch_timer_.startTime();
}

void sentinel::worker_manager::Server::AssignTask(uint32_t task_id) {
    //Spawn a new thread if there are any available in the pool
    if(pool_.Size() < pool_.MaxSize()) {
        pool_.Assign();
    }

    //Enqueue work in existing thread
    std::shared_ptr<Worker> thread = FindMinimumQueue();
    thread->Enqueue(task_id);
    ++num_tasks_assigned_;

    //Update Job Manager
    if(ReadyToUpdateJobManager()) {
        UpdateJobManager();
    }
}

void sentinel::worker_manager::Server::Terminate() {
    exit(0);
}

void sentinel::worker_manager::Server::Finalize() {
    pool_.StopAll();
}

/*
 * THREAD
 * */

sentinel::worker_manager::Worker::Worker() {
}

int sentinel::worker_manager::Worker::GetTask() {
    int task_id = queue_.front();
    queue_.pop_front();
    return task_id;
}

void sentinel::worker_manager::Worker::ExecuteTask(int task_id) {
}

void sentinel::worker_manager::Worker::Run(std::future<void> loop_cond) {
    /**
     * TODO: avoid variables such as queue_ please name is full.
     */
    bool kill_if_empty = false;
    do {
        if(queue_.size() == 0 && kill_if_empty) {
            return;
        }
        while (queue_.size() > 0) {
            ExecuteTask(GetTask());
        }
        kill_if_empty = true;
    }
    while(loop_cond.wait_for(std::chrono::milliseconds(1))==std::future_status::timeout);
}

void sentinel::worker_manager::Worker::Enqueue(int task_id) {
    queue_.emplace_back(task_id);
}

int sentinel::worker_manager::Worker::GetQueueDepth() {
    return queue_.size();
}