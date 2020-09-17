#include <fstream>
#include <string>
#include <iostream>
#include <sentinel/job_manager/client.h>

int main(int argc, char * argv[]){
    MPI_Init(&argc,&argv);
    MPI_Barrier(MPI_COMM_WORLD);

    if(argc > 1) {
        SENTINEL_CONF->CONFIGURATION_FILE = argv[1];
    }

    auto client = basket::Singleton<sentinel::job_manager::client>::GetInstance();
    MPI_Barrier(MPI_COMM_WORLD);
    client->SubmitJob(0);
}