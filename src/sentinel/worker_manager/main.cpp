//
// Created by lukemartinlogan on 9/16/20.
//

#include <basket/common/singleton.h>
#include <common/daemon.h>
#include <common/arguments.h>
#include <sentinel/common/configuration_manager.h>
#include "server.h"
#include <mpi.h>

class WorkerManagerArgs : public common::args::ArgMap {
private:
    void VerifyArgs(void) {}

public:
    void Usage(void) {
        std::cout << "Usage: ./worker_manager -[param-id] [value] ... " << std::endl;
        std::cout << "[string]: The config file for sentinel. Default is no config." << std::endl;
    }

    WorkerManagerArgs(int argc, char **argv) {
        AddOpt("", common::args::ArgType::kString, "");
        ArgIter(argc, argv);
        VerifyArgs();
    }
};

int main(int argc, char **argv) {
    MPI_Init(&argc,&argv);
    WorkerManagerArgs args(argc, argv);
    SENTINEL_CONF->CONFIGURATION_FILE = args.GetStringOpt("");
    auto daemon = basket::Singleton<common::Daemon<sentinel::worker_manager::Server>>::GetInstance();
    daemon->Run();
    MPI_Finalize();
    return 0;
}