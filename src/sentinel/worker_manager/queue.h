//
// Created by lukemartinlogan on 9/3/20.
//

#ifndef SENTINEL_QUEUE_H
#define SENTINEL_QUEUE_H

#include <common/debug.h>
#include <thread>
#include <mutex>
#include <list>
#include <atomic>

namespace sentinel {

template<typename T>
class Queue {
private:
    std::mutex lock_;
    std::list<T> list_;
    uint32_t size_;
public:
    Queue():lock_(),list_(),size_(0){}
    void Push(const T &obj) {
        lock_.lock();
        list_.push_back(obj);
        ++size_;
        lock_.unlock();
    }
    bool Front(T &obj) {
        if(size_ == 0) {
            return false;
        }
        lock_.lock();
        obj = list_.front();
        lock_.unlock();
        return true;
    }
    bool Pop(T &obj) {
        if(size_ == 0) {
            return false;
        }
        lock_.lock();
        obj = list_.front();
        list_.pop_front();
        --size_;
        lock_.unlock();
        return true;
    }
    uint32_t Size() {
        return size_;
    }
};

}
#endif //SENTINEL_QUEUE_H
