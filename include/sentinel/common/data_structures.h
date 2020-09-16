//
// Created by mani on 9/14/2020.
//

#ifndef RHEA_DATA_STRUCTURES_H
#define RHEA_DATA_STRUCTURES_H


#include <basket/common/data_structures.h>
#include <rpc/msgpack.hpp>

#include <basket.h>
#include <common/data_structure.h>
#include <vector>


typedef struct Task{

    std::vector<std::shared_ptr<Task>> links;

    Task():links(){}

    void Execute(){
        std::cout << "Test task's execute function...." << std::endl;
    }
}Task;

typedef struct Job{

    std::shared_ptr<Task> source_;

    Job(): source_(){}
    Job(const Job &other): source_(other.source_) {}
    Job(Job &other): source_(other.source_) {}
    /*Define Assignment Operator*/
    Job &operator=(const Job &other){
        source_ = other.source_;
        return *this;
    }
    std::shared_ptr<Task> GetTask(uint32_t task_id_){
        printf("Begin to create Task....\n");
        return std::make_shared<Task>();
    }

    uint32_t GetNextTaskId(uint32_t task_id_){
        printf("Test Job's test function....\n");
        return task_id_ + 1;
    }

}Job;


typedef struct ResourceAllocation {

    CharStruct newWorkermanagerNodes; // for file io, the "id_" is the filename; for object store io, the "id_" is the key.
    uint16_t numNewWorkermanager;

    /*Define the default, copy and move constructor*/
    ResourceAllocation(): newWorkermanagerNodes(), numNewWorkermanager(0){}
    ResourceAllocation(const ResourceAllocation &other): newWorkermanagerNodes(other.newWorkermanagerNodes),
                                                         numNewWorkermanager(other.numNewWorkermanager){}
    ResourceAllocation(ResourceAllocation &other): newWorkermanagerNodes(other.newWorkermanagerNodes),
                                                   numNewWorkermanager(other.numNewWorkermanager){}

    /*Define Assignment Operator*/
    ResourceAllocation &operator=(const ResourceAllocation &other){
        newWorkermanagerNodes = other.newWorkermanagerNodes;
        numNewWorkermanager = other.numNewWorkermanager;
        return *this;
    }
} ResourceAllocation;

typedef struct WorkerManagerStats {

    double thrpt_kops_;
    uint32_t num_tasks_exec_;
    uint32_t num_tasks_queued_;
    WorkerManagerStats():thrpt_kops_(0),num_tasks_exec_(0),num_tasks_queued_(0){}
    WorkerManagerStats(double epoch_time, int num_tasks_assigned, int num_tasks_queued) {
        num_tasks_queued_ = num_tasks_queued;
        num_tasks_exec_ = num_tasks_assigned - num_tasks_queued_;
        thrpt_kops_ = num_tasks_exec_ / epoch_time;
    }
    /*Define the default, copy and move constructor*/
    WorkerManagerStats(const WorkerManagerStats &other): thrpt_kops_(other.thrpt_kops_), num_tasks_exec_(other.num_tasks_exec_), num_tasks_queued_(other.num_tasks_queued_) {}
    WorkerManagerStats(WorkerManagerStats &other):  thrpt_kops_(other.thrpt_kops_), num_tasks_exec_(other.num_tasks_exec_), num_tasks_queued_(other.num_tasks_queued_) {}
    /*Define Assignment Operator*/
    WorkerManagerStats &operator=(const WorkerManagerStats &other){
        thrpt_kops_ = other.thrpt_kops_;
        num_tasks_exec_ = other.num_tasks_exec_;
        num_tasks_queued_ = other.num_tasks_queued_;
        return *this;
    }
} WorkerManagerStats;


std::ostream &operator<<(std::ostream &os, Data &data);

#endif //RHEA_DATA_STRUCTURES_H
