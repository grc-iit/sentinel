//
// Created by mani on 9/14/2020.
//

#ifndef RHEA_JOB_MANAGER_H
#define RHEA_JOB_MANAGER_H

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

typedef uint32_t workmanager_id, task_id;

namespace sentinel::job_manager{
    class Server {
    private:
        std::shared_ptr<RPC> rpc;
        std::unordered_map<workmanager_id, WorkerManagerStats> loadMap;
        std::unordered_map<workmanager_id, std::vector<task_id>> taskMap;
        std::unordered_map<task_id, std::pair<workmanager_id, task_id>> destinationMap;
        std::shared_ptr<Job> job;
        workmanager_id FindMinLoad();
        bool SpawnTaskManagers(ResourceAllocation &resourceAllocation);

        void RunInternal(std::future<void> futureObj);
    public:
        void Run(std::future<void> futureObj);

        Server(){
            SENTINEL_CONF->ConfigureJobManagerServer();
            auto basket=BASKET_CONF;
            rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);
            std::function<bool(uint32_t)> functionSubmitJob(std::bind(&sentinel::job_manager::Server::SubmitJob, this, std::placeholders::_1));
            std::function<bool(uint32_t,WorkerManagerStats&)> functionUpdateWorkerManagerStats(std::bind(&sentinel::job_manager::Server::UpdateWorkerManagerStats, this, std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool, WorkerManagerStats>(uint32_t)> functionGetWorkerManagerStats(std::bind(&sentinel::job_manager::Server::GetWorkerManagerStats, this, std::placeholders::_1));
            std::function<std::pair<uint32_t, uint32_t>(uint32_t)> functionGetNextNode(std::bind(&sentinel::job_manager::Server::GetNextNode, this, std::placeholders::_1));
            std::function<bool(ResourceAllocation&)> functionChangeResourceAllocation(std::bind(&sentinel::job_manager::Server::ChangeResourceAllocation, this, std::placeholders::_1));
            rpc->bind("SubmitJob", functionSubmitJob);
            rpc->bind("UpdateWorkerManagerStats", functionUpdateWorkerManagerStats);
            rpc->bind("GetWorkerManagerStats", functionGetWorkerManagerStats);
            rpc->bind("GetNextNode", functionGetNextNode);
            rpc->bind("ChangeResourceAllocation", functionChangeResourceAllocation);
        }
        bool SubmitJob(uint32_t jobId);
        bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats);
        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);
        std::pair<workmanager_id, task_id> GetNextNode(uint32_t currentTaskId);
        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);
    };
}


#endif //RHEA_JOB_MANAGER_H
