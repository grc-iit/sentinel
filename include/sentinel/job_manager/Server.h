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

typedef uint32_t job_id, workmanager_id, task_id;

namespace sentinel::job_manager{
    class Server {
    private:
        std::shared_ptr<RPC> rpc;
        std::unordered_map<workmanager_id, WorkerManagerStats> loadMap;
        std::map<WorkerManagerStats, workmanager_id> reversed_loadMap;
        std::mutex mtx_loadmap;

        std::unordered_map<workmanager_id, std::vector<task_id>> taskMap;
        std::unordered_map<task_id, std::pair<workmanager_id, task_id>> destinationMap;

        std::mutex mtx_allocate;
        std::unordered_map<workmanager_id, CharStruct> available_workermanagers;
        std::unordered_map<job_id, std::vector<workmanager_id>> used_resources;
        std::unordered_map<workmanager_id, job_id> reversed_used_resources;

        std::unordered_map<job_id, std::shared_ptr<Job>> jobs;
        bool SpawnWorkerManagers(ResourceAllocation &resourceAllocation);
        bool TerminateWorkerManagers(ResourceAllocation &resourceAllocation);

        void RunInternal(std::future<void> futureObj);
    public:
        void Run(std::future<void> futureObj);

        Server(){
            SENTINEL_CONF->ConfigureJobManagerServer();
            auto basket=BASKET_CONF;
            rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
            std::function<bool(uint32_t)> functionSubmitJob(std::bind(&sentinel::job_manager::Server::SubmitJob, this, std::placeholders::_1));
            std::function<bool(uint32_t)> functionTerminateJob(std::bind(&sentinel::job_manager::Server::TerminateJob, this, std::placeholders::_1));
            std::function<bool(uint32_t,WorkerManagerStats&)> functionUpdateWorkerManagerStats(std::bind(&sentinel::job_manager::Server::UpdateWorkerManagerStats, this, std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool, WorkerManagerStats>(uint32_t)> functionGetWorkerManagerStats(std::bind(&sentinel::job_manager::Server::GetWorkerManagerStats, this, std::placeholders::_1));
            std::function<std::pair<uint32_t, uint32_t>(uint32_t, uint32_t)> functionGetNextNode(std::bind(&sentinel::job_manager::Server::GetNextNode, this, std::placeholders::_1, std::placeholders::_2));
            std::function<bool(ResourceAllocation&)> functionChangeResourceAllocation(std::bind(&sentinel::job_manager::Server::ChangeResourceAllocation, this, std::placeholders::_1));
            rpc->bind("SubmitJob", functionSubmitJob);
            rpc->bind("TerminateJob", functionTerminateJob);
            rpc->bind("UpdateWorkerManagerStats", functionUpdateWorkerManagerStats);
            rpc->bind("GetWorkerManagerStats", functionGetWorkerManagerStats);
            rpc->bind("GetNextNode", functionGetNextNode);
            rpc->bind("ChangeResourceAllocation", functionChangeResourceAllocation);

            int i = 0;
            for(auto&& node: SENTINEL_CONF->WORKERMANAGER_LISTS){
                available_workermanagers.insert(std::make_pair(i, node));
                i++;
            }
        }
        bool SubmitJob(uint32_t jobId);
        bool TerminateJob(uint32_t jobId);
        bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats);
        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);
        std::pair<workmanager_id, task_id> GetNextNode(uint32_t workermanagerId, uint32_t currentTaskId);
        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);
    };
}


#endif //SENTINEL_JOB_MANAGER_SERVER_H
