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

sentinel::worker_manager::Server::Server() : num_tasks_assigned_(0) {
    AUTO_TRACER("sentinel::worker_manager::Server::Server");
    job_manager = basket::Singleton<sentinel::job_manager::client>::GetInstance();
    pool_.Init(SENTINEL_CONF->WORKERTHREAD_COUNT);
    epoch_msec_ = SENTINEL_CONF->WORKERMANAGER_EPOCH_MS;
    min_tasks_assigned_update_ = SENTINEL_CONF->WORKERMANAGER_UPDATE_MIN_TASKS;
    epoch_timer_.startTime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    Init();
    int i=0;
    while(pool_.Size() < pool_.MaxSize()) {
        pool_.Assign(i,this, i++);
    }
    worker_manager = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();
}

void sentinel::worker_manager::Server::Init() {
    AUTO_TRACER("sentinel::worker_manager::Server::Init");
    SENTINEL_CONF->ConfigureWorkermanagerServer();
    client_rpc_ = basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

    std::function<ThreadId(std::set<ThreadId> threads,uint32_t,uint32_t,Event &)> functionAssignTask(std::bind(
            &sentinel::worker_manager::Server::AssignTask,
            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    client_rpc_->bind("AssignTask", functionAssignTask);

    std::function<bool(void)>  functionFinalizeWorkerManager(std::bind(&sentinel::worker_manager::Server::FinalizeWorkerManager, this));
    client_rpc_->bind("FinalizeWorkerManager", functionFinalizeWorkerManager);
}

void sentinel::worker_manager::Server::Run(std::future<void> loop_cond, common::Daemon<Server>* obj) {
    daemon = obj;
    AUTO_TRACER("sentinel::worker_manager::Server::Run");
    while(loop_cond.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {}
}

std::shared_ptr<sentinel::worker_manager::Worker>
sentinel::worker_manager::Server::FindMinimumQueue(std::set<ThreadId> &threads) {
    AUTO_TRACER("sentinel::worker_manager::Server::FindMinimumQueue",threads);
    std::shared_ptr<Worker> worker;
    size_t min_queue_size = std::numeric_limits<size_t>::max();
    if(threads.size() == 0) {
        for(uint16_t i = 0; i < pool_.MaxSize(); ++i) {
            Thread<Worker> &thread = pool_.Get(i);
            if(thread.IsActive()) {
                std::shared_ptr<Worker> temp = std::move(thread.GetObj());
                if(temp->GetQueueDepth() < min_queue_size) {
                    min_queue_size = temp->GetQueueDepth();
                    worker = std::move(temp);
                }
            }
        }
    }else{
        for(const auto& thread_id: threads){
            Thread<Worker> &thread = pool_.Get(thread_id);
            if(!thread.IsActive()) {
                thread.Assign(this, thread_id);
            }
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
    auto check = job_manager->UpdateWorkerManagerStats(rank_, wms);
    epoch_timer_.startTime();
    num_tasks_assigned_ = 0;
    return check;
}

ThreadId sentinel::worker_manager::Server::AssignTask(std::set<ThreadId> threads, JobId job_id, TaskId task_id, Event &event) {
    AUTO_TRACER("sentinel::worker_manager::Server::AssignTask", threads,task_id,task_id,event);
    //Enqueue work in existing thread
    std::shared_ptr<Worker> thread = FindMinimumQueue(threads);
    thread->Enqueue(std::tuple<uint32_t,uint32_t,Event>(job_id,task_id,event));
    ++num_tasks_assigned_;

    //Update Job Manager
    if(ReadyToUpdateJobManager()) {
        UpdateJobManager();
    }
    return thread->id_;
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
    queue_.Front(id);
    return std::move(id);
}

void sentinel::worker_manager::Worker::ExecuteTask(std::tuple<uint32_t,uint32_t,Event> id,std::future<void> loop_cond) {
    AUTO_TRACER("sentinel::worker_manager::Worker::ExecuteTask");
    std::shared_ptr<Job<Event>> job = ClassLoader().LoadClass<Job<Event>>(std::get<0>(id));
    std::shared_ptr<Task<Event>> task = job->GetTask(std::get<1>(id));
    std::function<bool(uint32_t, uint32_t, Event &)> emit_function(std::bind(&sentinel::worker_manager::Worker::EmitCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    task->EmitCallback(emit_function);
    task->EndLoop(std::move(loop_cond));
    switch(task->type_){
        case TaskType::SOURCE:{
            task->Execute(std::get<2>(id));
            break;
        }
        case TaskType::KEYBY:{
            Event hash_event = task->Execute(std::get<2>(id));
            emit_function(task->job_id_,task->id_,hash_event);
            break;
        }
        case TaskType::SINK:{
            task->Execute(std::get<2>(id));
            break;
        }
    }
}

void sentinel::worker_manager::Worker::GetAndExecuteTask(std::future<void> loop_cond) {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetAndExecuteTask");
    ExecuteTask(GetTask(),std::move(loop_cond));
    std::tuple<uint32_t,uint32_t,Event> id;
    queue_.Pop(id);
}

void sentinel::worker_manager::Worker::Run(std::future<void> loop_cond,Server* server, ThreadId id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Run");
    server_=server;
    id_=id;
    pthread_setname_np(pthread_self(), std::to_string(id_).c_str());
    do {
        while (queue_.Size() > 0) {
            GetAndExecuteTask(std::move(loop_cond));
        }
    } while(loop_cond.wait_for(std::chrono::milliseconds(thread_timeout_ms_)) == std::future_status::timeout);
}

void sentinel::worker_manager::Worker::Enqueue(std::tuple<uint32_t,uint32_t,Event> id) {
    AUTO_TRACER("sentinel::worker_manager::Worker::Enqueue",id);
    queue_.Push(id);
}

int sentinel::worker_manager::Worker::GetQueueDepth() {
    AUTO_TRACER("sentinel::worker_manager::Worker::GetQueueDepth");
    return queue_.Size();
}

bool sentinel::worker_manager::Worker::EmitCallback(uint32_t job_id, uint32_t current_task_id, Event &output_event) {
    AUTO_TRACER("sentinel::worker_manager::Worker::EmitCallback",job_id,current_task_id,output_event);
    auto next_tasks = server_->job_manager->GetNextNode(job_id, current_task_id,output_event);
    for(auto next_task:next_tasks){
        uint32_t worker_index=std::get<0>(next_task);
        auto threads=std::get<1>(next_task);
        uint32_t next_task_id=std::get<2>(next_task);
        auto my_id = SENTINEL_CONF->WORKERMANAGER_ID;
        if(my_id == worker_index){
            server_->AssignTask(threads,job_id,next_task_id,output_event);
        }else{
            server_->worker_manager->AssignTask(worker_index,threads,job_id,next_task_id, output_event);
        }
    }
    return true;
}
