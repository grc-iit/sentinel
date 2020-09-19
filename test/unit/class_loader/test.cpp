//
// Created by yejie on 9/16/20.
//

#include <common/class_loader.h>
#include <sentinel/common/data_structures.h>

int main(int argc, char* argv[]){
    ClassLoader class_loader;
    // load so file
    int class_id = 1;
    int task_id = 0;
    if(argc > 1){
        COMMON_CONF->JOB_PATH=argv[1];
        class_id = std::stoi(argv[2]);
    }

    auto start_time = std::chrono::steady_clock::now();
    std::shared_ptr<Job> job = class_loader.LoadClass<Job>(class_id);
    auto end_time = std::chrono::steady_clock::now();
    std::shared_ptr<Task> task = job->GetTask(task_id);
    task->Execute();

    double dr_ns=std::chrono::duration<double,std::milli>(end_time-start_time).count();
    printf("It spend %0.2f ms to load all the so files\n", dr_ns);
}

