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
#include "../../../../scs_io_common/include/common/class_loader.h" //FIXME: Reference Issue 1 on scs-lab:sentinel
#include "../../../../scs_io_common/include/common/data_structure.h"

typedef uint32_t workmanager_id, task_id;

namespace sentinel::job_manager{
    class server {
    private:
        std::shared_ptr<RPC> rpc;
        std::unordered_map<workmanager_id, WorkerManagerStats> loadMap;
        std::unordered_map<workmanager_id, std::vector<task_id>> taskMap;
        std::unordered_map<task_id, std::pair<workmanager_id, task_id>> destinationMap;

        Job job;

        workmanager_id findMinLoad();
        bool sentinel::job_manager::server::SpawnTaskManagers(ResourceAllocation &resourceAllocation);

        void RunInternal(std::future<void> futureObj);
    public:
        void Run(std::future<void> futureObj);
        explicit server();

        bool SubmitJob(uint32_t jobId);
        bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats);
        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);
        std::pair<workmanager_id, task_id> GetNextNode(uint32_t currentTaskId);
        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);
    };
}


#endif //RHEA_JOB_MANAGER_H
