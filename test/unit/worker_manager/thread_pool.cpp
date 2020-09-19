//
// Created by lukemartinlogan on 9/18/20.
//

#include <sentinel/worker_manager/thread_pool.h>
#include <sentinel/worker_manager/server.h>

int main()
{
    int tp_sz = 16;
    sentinel::ThreadPool<sentinel::worker_manager::Worker> pool;
    pool.Init(tp_sz);
    for(int i = 0; i < tp_sz; ++i) {
        pool.Assign();
    }
    for(int i = 0; i < tp_sz; ++i) {
        auto worker = pool.Get(i);
        Event e;
        worker->Enqueue(std::tuple<uint32_t,uint32_t,Event>(i,i,e));
    }
    pool.WaitAll();
}