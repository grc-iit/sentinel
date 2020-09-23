//
// Created by mani on 9/14/2020.
//

#ifndef SENTINEL_JOB_MANAGER_SERVER_H
#define SENTINEL_JOB_MANAGER_SERVER_H

#include <mpi.h>
#include <basket/communication/rpc_lib.h>
#include <basket/communication/rpc_factory.h>
#include <sentinel/common/data_structures.h>
#include <sentinel/common/configuration_manager.h>
#include <sentinel/common/debug.h>
#include <rpc/client.h>
#include <common/class_loader.h>
#include <common/data_structure.h>
#include <future>
#include <common/data_structure.h>
#include <sentinel/worker_manager/client.h>
#include <basket/common/singleton.h>
#include <common/daemon.h>

typedef uint32_t job_id, workmanager_id, task_id;

namespace sentinel::job_manager{
    class Server {
    private:
        common::Daemon<Server> * daemon;
        std::shared_ptr<RPC> rpc;
        std::unordered_map<workmanager_id, WorkerManagerStats> loadMap;
        std::map<WorkerManagerStats, workmanager_id> reversed_loadMap;
        std::mutex mtx_loadmap;

        std::shared_ptr<sentinel::worker_manager::Client> workermanager_client;

        std::unordered_map<workmanager_id, std::vector<task_id>> taskMap;
        std::unordered_map<task_id, std::pair<workmanager_id, task_id>> destinationMap;

        std::mutex mtx_allocate;
        std::mutex mtx_resources;
        std::unordered_map<workmanager_id, std::pair<CharStruct,uint32_t>> available_workermanagers;
        std::unordered_map<job_id, std::vector<std::tuple<workmanager_id,uint32_t,uint32_t>>> used_resources;

        std::unordered_map<job_id, std::shared_ptr<Job<Event>>> jobs;
        bool SpawnWorkerManagers(uint32_t required_threads, job_id job_id_);
        bool TerminateWorkerManagers(ResourceAllocation &resourceAllocation);

        void RunInternal(std::future<void> futureObj);
    public:
        void Run(std::future<void> futureObj,common::Daemon<Server> * daemon);

        Server(){
            SENTINEL_CONF->ConfigureJobManagerServer();
            auto basket=BASKET_CONF;
            rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

            std::function<bool(uint32_t,uint32_t)> functionSubmitJob(std::bind(&sentinel::job_manager::Server::SubmitJob, this, std::placeholders::_1, std::placeholders::_2));
            std::function<bool(uint32_t)> functionTerminateJob(std::bind(&sentinel::job_manager::Server::TerminateJob, this, std::placeholders::_1));
            std::function<bool(uint32_t,WorkerManagerStats&)> functionUpdateWorkerManagerStats(std::bind(&sentinel::job_manager::Server::UpdateWorkerManagerStats, this, std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool, WorkerManagerStats>(uint32_t)> functionGetWorkerManagerStats(std::bind(&sentinel::job_manager::Server::GetWorkerManagerStats, this, std::placeholders::_1));
            std::function<std::vector<std::tuple<uint32_t, uint16_t, task_id>>(uint32_t job_id, uint32_t currentTaskId, Event e)> functionGetNextNode(std::bind(&sentinel::job_manager::Server::GetNextNode, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            std::function<bool(ResourceAllocation&)> functionChangeResourceAllocation(std::bind(&sentinel::job_manager::Server::ChangeResourceAllocation, this, std::placeholders::_1));
            rpc->bind("SubmitJob", functionSubmitJob);
            rpc->bind("TerminateJob", functionTerminateJob);
            rpc->bind("UpdateWorkerManagerStats", functionUpdateWorkerManagerStats);
            rpc->bind("GetWorkerManagerStats", functionGetWorkerManagerStats);
            rpc->bind("GetNextNode", functionGetNextNode);
            rpc->bind("ChangeResourceAllocation", functionChangeResourceAllocation);

            workermanager_client = basket::Singleton<sentinel::worker_manager::Client>::GetInstance();

            int i = 0;
            for(auto&& node: SENTINEL_CONF->WORKERMANAGER_LISTS){
                available_workermanagers.insert({i, {node,SENTINEL_CONF->WORKERTHREAD_COUNT}});
                i++;
            }
        }
        bool SubmitJob(uint32_t jobId, uint32_t num_sources);
        bool TerminateJob(uint32_t jobId);
        bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats);
        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);
        std::vector<std::tuple<uint32_t, uint16_t, task_id>> GetNextNode(uint32_t job_id, uint32_t currentTaskId, Event e);
        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);
    };
}


#endif //SENTINEL_JOB_MANAGER_SERVER_H
