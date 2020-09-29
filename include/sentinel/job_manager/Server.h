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
#include <common/debug.h>
#include <rpc/client.h>
#include <common/class_loader.h>
#include <common/data_structure.h>
#include <future>
#include <common/data_structure.h>
#include <sentinel/worker_manager/client.h>
#include <basket/common/singleton.h>
#include <common/daemon.h>
#include <sentinel/common/typedefs.h>



namespace sentinel::job_manager{
    class Server {
    public:
        common::Daemon<Server> * daemon;
        std::shared_ptr<RPC> rpc;
        std::shared_ptr<sentinel::worker_manager::Client> workermanager_client;
        mutable std::shared_mutex load_mutex_, job_mutex_, resources_mutex_;


        // Maintains load of each worker manager
        std::unordered_map<WorkerManagerId, WorkerManagerStats> loadMap;
        // Maintains lowest load worker on top
        std::multimap<WorkerManagerStats, WorkerManagerId> reversed_loadMap;
        std::vector<WorkerManagerResource> worker_managers;
        // Maintains available resources per worker manager instance
        std::unordered_map<WorkerManagerId, WorkerManagerResource> available_workermanagers;
        // Maintains resources allocated per job
        std::unordered_map<JobId, std::vector<WorkerManagerResource>> used_resources;
        // Maintains loaded job per id
        std::unordered_map<JobId, std::shared_ptr<Job<Event>>> jobs;


        bool SpawnWorkerManagers(ThreadId required_threads, JobId job_id);
        bool TerminateWorkerManagers(ResourceAllocation &resourceAllocation);

        void RunInternal(std::future<void> futureObj);

        void Run(std::future<void> futureObj,common::Daemon<Server> * daemon);

        Server():worker_managers(){
            SENTINEL_CONF->ConfigureJobManagerServer();
            auto basket=BASKET_CONF;
            rpc=basket::Singleton<RPCFactory>::GetInstance()->GetRPC(BASKET_CONF->RPC_PORT);

            std::function<bool(JobId,TaskId,uint32_t)> functionSubmitJob(std::bind(&sentinel::job_manager::Server::SubmitJob, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            std::function<bool(JobId)> functionTerminateJob(std::bind(&sentinel::job_manager::Server::TerminateJob, this, std::placeholders::_1));
            std::function<bool(WorkerManagerId,WorkerManagerStats&)> functionUpdateWorkerManagerStats(std::bind(&sentinel::job_manager::Server::UpdateWorkerManagerStats, this, std::placeholders::_1, std::placeholders::_2));
            std::function<std::pair<bool, WorkerManagerStats>(WorkerManagerId)> functionGetWorkerManagerStats(std::bind(&sentinel::job_manager::Server::GetWorkerManagerStats, this, std::placeholders::_1));
            std::function<std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>>(JobId, TaskId, Event&)> functionGetNextNode(std::bind(&sentinel::job_manager::Server::GetNextNode, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
                WorkerManagerResource resource;
                resource.id_=i;
                resource.node_name_=node;
                resource.port_=SENTINEL_CONF->WORKERMANAGER_PORT_SERVER+i;
                for(int i=0;i<SENTINEL_CONF->WORKERTHREAD_COUNT;++i)
                    resource.threads_.insert(i);
                available_workermanagers.insert({i, resource});
                worker_managers.emplace_back(resource);
                i++;
            }
        }
        bool SubmitJob(JobId jobId, TaskId num_sources, uint32_t num_collectors_per_sources);
        bool TerminateJob(JobId jobId);
        bool UpdateWorkerManagerStats(WorkerManagerId workerManagerId, WorkerManagerStats &stats);
        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(WorkerManagerId workerManagerId);
        std::vector<std::tuple<JobId , std::set<ThreadId>, TaskId>> GetNextNode(JobId job_id, TaskId currentTaskId, Event &event);
        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);
    };
}


#endif //SENTINEL_JOB_MANAGER_SERVER_H
