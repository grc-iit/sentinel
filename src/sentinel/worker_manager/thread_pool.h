//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef RHEA_THREAD_POOL_H
#define RHEA_THREAD_POOL_H

#include <list>
#include <vector>
#include <thread>
#include <future>
#include <mutex>

namespace sentinel {
template<class T> class Thread;
template<class T> class ThreadPool;

template<class T>
class Thread {
private:
    ThreadPool<T> *pool_;
    int tid_;
    std::promise<void> loop_cond_;
    std::future<void> thread_complete_;
    std::shared_ptr<T> obj_;
public:
    Thread(ThreadPool<T> *pool, int tid) : pool_(pool), tid_(tid) {}

    template<typename ...Args>
    void Assign(Args ...args) {
        loop_cond_ = std::promise<void>();
        thread_complete_ = std::async(&Thread::Run, this, args...);
    }

    template<typename ...Args>
    void Run(Args ...args) {
        try {
            obj_->Run(std::move(loop_cond_.get_future()), args...);
        }
        catch(...) {
            pool_->Stop();
            throw 1; //TODO: Re-throw exception
        }
        pool_->Stop();
    }

    bool IsActive() {
        return
            thread_complete_.valid() &&
            thread_complete_.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready;
    }

    void Stop() {
        loop_cond_.set_value();
        loop_cond_ = std::promise<void>();
    }

    int GetTid() {
        return tid_;
    }

    std::shared_ptr<T> GetObj() {
        return obj_;
    }
};

template<class T>
class ThreadPool {
private:
    int size_;
    std::vector<Thread<T>> pool_;
    std::list<Thread<T>*> free_list_;
    std::mutex lock;
public:
    ThreadPool() = default;
    void Init(int count)  {
        pool_.resize(count);
        for(int i = 0; i < count; ++i) {
            pool_.emplace_back(this, i);
            free_list_.emplace_back(&pool_[i]);
        }
    }

    template<typename ...Args>
    int Assign(Args ...args) {
        if(Size() == pool_.size()) {
            throw 1; //TODO: Actual exception
        }
        lock.lock();
        Thread<T> *thread = free_list_.front();
        ++size_;
        lock.unlock();
        thread->Assign(args...);
        return thread->GetTid();
    }

    void Stop(int tid) {
        pool_[tid].Stop();
        lock.lock();
        free_list_.emplace_back(&pool_[tid]);
        --size_;
        lock.unlock();
    }

    void StopAll() {
        for(int i = 0; i < pool_.size(); ++i) {
            Stop(i);
        }
    }

    int Size() {
        return size_;
    }

    int MaxSize() {
        return pool_.size();
    }

    typename std::vector<Thread<T>>::iterator begin() {
        return pool_.begin();
    }

    typename std::vector<Thread<T>>::iterator end() {
        return pool_.end();
    }
};

}
#endif //RHEA_THREAD_POOL_H
