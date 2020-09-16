#include <mpi.h>
#include <basket.h>
#include <symbios/server/daemon.h>
#include <symbios/server/server.h>
#include <symbios/common/configuration_manager.h>

int main(int argc, char* argv[]){
    MPI_Init(&argc,&argv);
    MPI_Barrier(MPI_COMM_WORLD);
    if(argc > 1) SYMBIOS_CONF->CONFIGURATION_FILE=argv[1];
    BASKET_CONF->BACKED_FILE_DIR=SYMBIOS_CONF->SERVER_DIR;
    CharStruct log = "./single_node_symbios_server.log";
    auto daemon = basket::Singleton<symbios::Daemon<symbios::Server>>::GetInstance(log);
    daemon->Run();
    return 0;
}