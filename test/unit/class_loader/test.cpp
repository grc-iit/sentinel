//
// Created by yejie on 9/16/20.
//

#include <common/class_loader.h>
#include <sentinel/common/data_structures.h>

int main(int argc, char* argv[]){
    ClassLoader class_loader;
    // load so file
    std::shared_ptr<Job> job = class_loader.LoadClass<Job>(1);
    std::shared_ptr<Task> task = job->GetTask(1);
    task->Execute();
    job->GetNextTaskId(1);

    std::shared_ptr<Job> job2 = class_loader.LoadClass<Job>(2);
}

