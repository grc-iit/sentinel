//
// Created by lukemartinlogan on 9/17/20.
//

#include <sentinel/worker_manager/client.h>
#include <common/arguments.h>

class TestArgs : public common::args::ArgMap {
private:
    void VerifyArgs(void) {}

public:
    void Usage(void) {
        std::cout << "Usage: ./test -[param-id] [value] ... " << std::endl;
        std::cout << "-conf [string]: The config file for sentinel. Default is no config." << std::endl;
    }

    TestArgs(int argc, char **argv) {
        AddOpt("-conf", common::args::ArgType::kString, "");
        ArgIter(argc, argv);
        VerifyArgs();
    }
};

int main(int argc, char **argv)
{
    MPI_Init(&argc,&argv);
    TestArgs args(argc, argv);
    SENTINEL_CONF->CONFIGURATION_FILE = args.GetStringOpt("-conf");
    sentinel::worker_manager::Client client;
    for(int i = 0; i < 100; ++i) {
        Event e;
        client.AssignTask(0,0, i, i,e);
    }
    sleep(1);
    client.FinalizeWorkerManager(0);
    MPI_Finalize();
    return 0;
}