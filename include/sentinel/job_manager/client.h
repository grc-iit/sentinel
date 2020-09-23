//
// Created by mani on 9/14/2020.
//

#ifndef SENTINEL_JOB_MANAGER_CLIENT_H
#define SENTINEL_JOB_MANAGER_CLIENT_H

#include <mpi.h>
#include <basket/communication/rpc_lib.h>
#include <basket/communication/rpc_factory.h>
#include <sentinel/common/data_structures.h>
#include <sentinel/common/configuration_manager.h>
#include <common/debug.h>
#include <rpc/client.h>

namespace sentinel::job_manager {
    class client {
    private:
        std::shared_ptr<RPC> rpc;
    public:
        client();

        bool SubmitJob(uint32_t jobId, uint32_t num_sources);

        bool TerminateJob(uint32_t jobId);

        bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats) ;

        std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);

        std::vector<std::tuple<uint32_t, uint16_t, uint16_t, uint32_t>> GetNextNode(uint32_t job_id, uint32_t currentTaskId, Event event);

        bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);

    };
}


#endif //SENTINEL_JOB_MANAGER_CLIENT_H
