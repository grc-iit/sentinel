#include <fstream>
#include <string>
#include <iostream>
#include <mpi.h>

int main(int argc, char * argv[]){
    MPI_Init(&argc,&argv);
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm intercomm;

    MPI_Info info;
    MPI_Info_create(&info);
    MPI_Info_set(info,"host","127.0.0.1,127.0.0.1,127.0.0.1");
    int errcodes[1];
    char * mpi_argv[2];
    char* cmd = "/home/jaime/CLionProjects/rhea/cmake-build-debug/sentinel/test/unit/mpi_comm_spawn_executable_test";
    mpi_argv[0] = "test";
    mpi_argv[1] = (char *)0;
    std::cout << mpi_argv[0] << std::endl;
    std::cout << mpi_argv[1] << std::endl;
    MPI_Comm_spawn(cmd, mpi_argv, 2, info, 0, MPI_COMM_WORLD, &intercomm, errcodes );
    for(int i=0; i < 1; i++){
        if( errcodes[i] != MPI_SUCCESS) return false;
    }

    MPI_Finalize();
}