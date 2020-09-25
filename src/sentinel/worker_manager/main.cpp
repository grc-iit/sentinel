//
// Created by lukemartinlogan on 9/16/20.
//

#include <basket/common/singleton.h>
#include <common/daemon.h>
#include <common/arguments.h>
#include <sentinel/common/configuration_manager.h>
#include "server.h"
#include <mpi.h>

int main(int argc, char **argv) {
    MPI_Init(&argc,&argv);
    std::string conf;
    uint16_t port;
    WorkerManagerId id;
    if(argc >= 4) {
        conf = argv[1];
        port = atoi(argv[2]);
        id = atoi(argv[3]);
        printf("Worker Manager %d Initialized with conf %s, port %d\n",id, conf.data(),port);
    }else{
        perror("Not enough arguments ./worker_manager <conf> <port> <worker_manager_id>\n");
        exit(EXIT_FAILURE);
    }
    COMMON_CONF->CONFIGURATION_FILE = conf;
    SENTINEL_CONF->CONFIGURATION_FILE = conf;
    COMMON_CONF->LoadConfiguration();
    SENTINEL_CONF->WORKERMANAGER_PORT_SERVER = port;
    SENTINEL_CONF->WORKERMANAGER_ID = id;
    auto daemon = basket::Singleton<common::Daemon<sentinel::worker_manager::Server>>::GetInstance();
    daemon->Run();
    MPI_Finalize();
    return 0;
}