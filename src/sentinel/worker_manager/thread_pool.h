//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef RHEA_THREAD_POOL_H
#define RHEA_THREAD_POOL_H

#include <list>
#include <vector>
#include <thread>
#include <future>

namespace sentinel {

class Thread {
private:
    int tid_;
    std::promise<void> promise_;
    std::thread thread_;
public:
    Thread(int tid) : tid_(tid) {}

    template<typename ...Args>
    void Assign(Args ...args) {
        thread_ = std::thread(args..., std::move(promise_.get_future()));
    }

    void Stop() {
        promise_.set_value();
        thread_.join();
    }

    int GetTid() {
        return tid_;
    }
};

class ThreadPool {
private:
    int size_;
    std::vector<Rhea::Thread> pool_;
    std::list<Rhea::Thread*> free_list_;
public:
    ThreadPool() = default;
    void Init(int count)  {
        pool_.resize(count);
        for(int i = 0; i < count; ++i) {
            pool_.emplace_back(i);
            free_list_.emplace_back(&pool_[i]);
        }
    }

    template<typename ...Args>
    int Assign(Args ...args) {
        if(Size() == pool_.size()) {
            throw 1; //TODO: Actual exception
        }
        Thread *thread = free_list_.front();
        thread->Assign(args...);
        free_list_.pop_front();
        ++size_;
        return thread->GetTid();
    }

    void Stop(int tid) {
        pool_[tid].Stop();
        free_list_.emplace_back(&pool_[tid]);
        --size_;
    }

    void StopAll() {
        for(int i = 0; i < pool_.size(); ++i) {
            pool_[i].Stop();
        }
    }

    int Size() {
        return size_;
    }

    int MaxSize() {
        return pool_.size();
    }
};

}
#endif //RHEA_THREAD_POOL_H
