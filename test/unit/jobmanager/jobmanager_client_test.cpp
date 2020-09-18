#include <mpi.h>

int main(int argc, char * argv[]){
    MPI_Init(&argc,&argv);
    MPI_Barrier(MPI_COMM_WORLD);

//    bool SubmitJob(uint32_t jobId);
//    bool TerminateJob(uint32_t jobId);
//    bool UpdateWorkerManagerStats(uint32_t workerManagerId, WorkerManagerStats &stats);
//    std::pair<bool, WorkerManagerStats> GetWorkerManagerStats(uint32_t workerManagerId);
//    std::pair<workmanager_id, task_id> GetNextNode(uint32_t workermanagerId, uint32_t currentTaskId);
//    bool ChangeResourceAllocation(ResourceAllocation &resourceAllocation);

    MPI_Finalize();
}
