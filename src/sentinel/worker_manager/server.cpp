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
#include <sentinel/worker_manager/client.h>

/*
 * SERVER
 * */

sentinel::worker_manager::Server::Server() {
    AUTO_TRACER("sentinel::worker_manager::Server::Server");
    SENTINEL_CONF->ConfigureWorkermanagerServer();
    pool_.Init(SENTINEL_CONF->WORKERTHREAD_COUNT);
    epoch_msec_ = SENTINEL_CONF->WORKERMANAGER_EPOCH_MS;
    min_tasks_assigned_update_ = SENTINEL_CONF->WORKERMANAGER_UPDATE_MIN_TASKS;
    epoch_timer_.startTime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    Init();
}

void sentinel::worker_manager::Server::Init() {
    AUTO_TRACER("sentinel::worker_manager::Server::Init");
    client_rpc_ = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

    std::function<bool(uint32_t, uint32_t,Event &)> functionAssignTask(std::bind(
            &sentinel::worker_manager::Server::AssignTask,
            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    client_rpc_->bind("AssignTask", functionAssignTask);

    std::function<bool(void)>  functionFinalizeWorkerManager(std::bind(&sentinel::worker_manager::Server::FinalizeWorkerManager, this));
    client_rpc_->bind("FinalizeWorkerManager", functionFinalizeWorkerManager);
}

void sentinel::worker_manager::Server::Run(std::future<void> loop_cond, common::Daemon<Server>* obj) {
    daemon = obj;
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
    epoch_timer_.pauseTime(); epoch_timer_.resumeTime();
    return (num_tasks_assigned_ > min_tasks_assigned_update_) ||
           (epoch_timer_.getTimeElapsed() >= epoch_msec_);
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

bool sentinel::worker_manager::Server::AssignTask(uint32_t job_id, uint32_t task_id, Event &event) {
    AUTO_TRACER("sentinel::worker_manager::Server::AssignTask", task_id);

    //Spawn a new thread if there are any available in the pool
    if(pool_.Size() < pool_.MaxSize()) {
        pool_.Assign(this);
    }

    //Enqueue work in existing thread
    std::shared_ptr<Worker> thread = FindMinimumQueue();
    thread->Enqueue(std::tuple<uint32_t,uint32_t,Event>(job_id, task_id,event));
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
    pool_.WaitAll();
    daemon->Stop();
    return true;
}

/*
 * THREAD
 * */

sentinel::worker_manager::Worker::Worker():server_() {
    AUTO_TRACER("sentinel::worker_manager::Worker::Worker");
    thread_timeout_ms_ = SENTINEL_CONF->WORKERTHREAD_TIMOUT_MS;
}

std::tuple<uint32_t,uint32_t,Event> sentinel::worker_manager::Worker::GetTask() {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetTask");
    std::tuple<uint32_t,uint32_t,Event> id;
    queue_.Pop(id);
    return std::move(id);
}

void sentinel::worker_manager::Worker::ExecuteTask(std::tuple<uint32_t,uint32_t,Event> id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::ExecuteTask", task_id);
    std::shared_ptr<Job<Event>> job = ClassLoader().LoadClass<Job<Event>>(std::get<0>(id));
    std::shared_ptr<Task<Event>> task = job->GetTask(std::get<1>(id));
    std::function<bool(uint32_t job_id, uint32_t current_task_id, Event &output_event)> emit_function(std::bind(&sentinel::worker_manager::Worker::EmitCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    task->EmitCallback(emit_function);
    switch(task->type_){
        case TaskType::SOURCE:{
            SourceTask<Event>* task_= (SourceTask<Event>*)((void*)&task);
            task_->Execute(std::get<2>(id));
        }
        case TaskType::KEYBY:{
            KeyByTask<Event>* task_= (KeyByTask<Event>*)((void*)&task);
            auto hash = task_->Execute(std::get<2>(id));
            Event hash_event;
            hash_event.id_=std::to_string(hash);
            emit_function(task_->job_id_,task_->id_,hash_event);
        }
        case TaskType::SINK:{
            SinkTask<Event>* task_= (SinkTask<Event>*)((void*)&task);
            task_->Execute(std::get<2>(id));
        }
    }
}

void sentinel::worker_manager::Worker::GetAndExecuteTask() {
    ExecuteTask(GetTask());
}

void sentinel::worker_manager::Worker::Run(std::future<void> loop_cond,Server* server) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Run", task_id);
    server_=server;
    bool kill_if_empty = false;
    do {
        if(queue_.Size() == 0) {
            if(kill_if_empty) { return; }
            kill_if_empty = true;
        }
        else {
            kill_if_empty = false;
        }
        while (queue_.Size() > 0) {
            GetAndExecuteTask();
        }
    } while(loop_cond.wait_for(std::chrono::milliseconds(thread_timeout_ms_))==std::future_status::timeout);
}

void sentinel::worker_manager::Worker::Enqueue(std::tuple<uint32_t,uint32_t,Event> id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Enqueue", task_id);
    queue_.Push(id);
}

int sentinel::worker_manager::Worker::GetQueueDepth() {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetQueueDepth");
    return queue_.Size();
}

bool sentinel::worker_manager::Worker::EmitCallback(uint32_t job_id, uint32_t current_task_id, Event &output_event) {
    auto next_tasks = basket::Singleton<sentinel::job_manager::client>::GetInstance()->GetNextNode(job_id, current_task_id,output_event);
    for(auto next_task:next_tasks){
        uint32_t worker_index=std::get<0>(next_task);
        uint32_t worker_thread_id=std::get<1>(next_task);
        uint32_t next_task_id=std::get<2>(next_task);
        if(BASKET_CONF->MPI_RANK == worker_index){
            server_->AssignTask(job_id,next_task_id,output_event);
        }else{
            basket::Singleton<sentinel::worker_manager::Client>::GetInstance()->AssignTask(worker_index, job_id,next_task_id, output_event);
        }
    }
    return true;
}
