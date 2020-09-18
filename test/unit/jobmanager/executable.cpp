#include <fstream>
#include <string>
#include <iostream>
#include <mpi.h>
#include <iterator>

int main(int argc, char * argv[]){
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::string input = "hello\n";
    std::ofstream out("/home/jaime/CLionProjects/rhea/cmake-build-debug/sentinel/test/unit/output"+std::to_string(rank)+".txt");
    for(int i = 1; i < argc; i++){
        out << argv[i] << std::endl;
    }
    out.close();
    MPI_Finalize();
    return 0;
}