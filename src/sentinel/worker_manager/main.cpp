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
    if(argc >= 2) {
        conf = argv[1];
    }
    COMMON_CONF->CONFIGURATION_FILE = conf;
    SENTINEL_CONF->CONFIGURATION_FILE = conf;
    COMMON_CONF->LoadConfiguration();
    auto daemon = basket::Singleton<common::Daemon<sentinel::worker_manager::Server>>::GetInstance();
    daemon->Run();
    MPI_Finalize();
    return 0;
}