//
// Created by lukemartinlogan on 9/16/20.
//

#ifndef SENTINEL_WORKER_MANAGER_CLIENT_H
#define SENTINEL_WORKER_MANAGER_CLIENT_H

#include <basket.h>
#include <memory>
#include <basket/communication/rpc_factory.h>
#include <rpc/client.h>
#include <basket.h>
#include <string>
#include <sentinel/common/configuration_manager.h>

namespace sentinel::worker_manager {
        class Client {
        private:
            std::shared_ptr<RPC> server_rpc;
        public:
            Client();

            void Init();

            bool AssignTask(int server_index, int task_id);

            bool TerminateWorkerManager(int server_index);

            bool FinalizeWorkerManager(int server_index);

            void Finalize();
        };
};

#endif //SENTINEL_WORKER_MANAGER_CLIENT_H
