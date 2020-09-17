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

    uint16_t num_nodes_;
    uint16_t num_procs_per_node;
    uint16_t num_threads_per_proc;

    /*Define the default, copy and move constructor*/
    ResourceAllocation(): num_nodes_(), num_procs_per_node(0), num_threads_per_proc(){}
    ResourceAllocation(const ResourceAllocation &other): num_nodes_(other.num_nodes_), num_procs_per_node(other.num_procs_per_node), num_threads_per_proc(other.num_threads_per_proc){}
    ResourceAllocation(ResourceAllocation &other): num_nodes_(other.num_nodes_), num_procs_per_node(other.num_procs_per_node), num_threads_per_proc(other.num_threads_per_proc){}

    /*Define Assignment Operator*/
    ResourceAllocation &operator=(const ResourceAllocation &other){
        num_nodes_ = other.num_nodes_;
        num_procs_per_node = other.num_procs_per_node;
        num_threads_per_proc = other.num_threads_per_proc;
        return *this;
    }
} ResourceAllocation;

typedef struct WorkerManagerStats {

    double thrpt_kops_;
    uint32_t num_tasks_exec_;
    uint32_t num_tasks_queued_;
    WorkerManagerStats():thrpt_kops_(0),num_tasks_exec_(0),num_tasks_queued_(0){}
    WorkerManagerStats(double epoch_time, int num_tasks_assigned, int num_tasks_queued) {
        thrpt_kops_ = num_tasks_exec_ / epoch_time;
        num_tasks_exec_ = num_tasks_assigned - num_tasks_queued;
        num_tasks_queued_ = num_tasks_queued;
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
namespace clmdep_msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
            namespace adaptor {
                namespace mv1 = clmdep_msgpack::v1;
                template<>
                struct convert<WorkerManagerStats> {
                    mv1::object const &operator()(mv1::object const &o, WorkerManagerStats &input) const {
                        input.thrpt_kops_ = o.via.array.ptr[0].as<double>();
                        input.num_tasks_exec_ = o.via.array.ptr[1].as<uint32_t>();
                        input.num_tasks_queued_ = o.via.array.ptr[2].as<uint32_t>();
                        return o;
                    }
                };

                template<>
                struct pack<WorkerManagerStats> {
                    template<typename Stream>
                    packer <Stream> &operator()(mv1::packer <Stream> &o, WorkerManagerStats const &input) const {
                        o.pack_array(3);
                        o.pack(input.thrpt_kops_);
                        o.pack(input.num_tasks_exec_);
                        o.pack(input.num_tasks_queued_);
                        return o;
                    }
                };

                template<>
                struct object_with_zone<WorkerManagerStats> {
                    void operator()(mv1::object::with_zone &o, WorkerManagerStats const &input) const {
                        o.type = type::ARRAY;
                        o.via.array.size = 3;
                        o.via.array.ptr = static_cast<clmdep_msgpack::object *>(o.zone.allocate_align(
                                sizeof(mv1::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(mv1::object)));
                        o.via.array.ptr[0] = mv1::object(input.thrpt_kops_, o.zone);
                        o.via.array.ptr[1] = mv1::object(input.num_tasks_exec_, o.zone);
                        o.via.array.ptr[2] = mv1::object(input.num_tasks_queued_, o.zone);
                    }
                };

                template<>
                struct convert<ResourceAllocation> {
                    mv1::object const &operator()(mv1::object const &o, ResourceAllocation &input) const {
                        input.num_nodes_ = o.via.array.ptr[0].as<uint16_t>();
                        input.num_procs_per_node = o.via.array.ptr[1].as<uint16_t>();
                        input.num_threads_per_proc = o.via.array.ptr[2].as<uint16_t>();
                        return o;
                    }
                };

                template<>
                struct pack<ResourceAllocation> {
                    template<typename Stream>
                    packer <Stream> &operator()(mv1::packer <Stream> &o, ResourceAllocation const &input) const {
                        o.pack_array(3);
                        o.pack(input.num_nodes_);
                        o.pack(input.num_procs_per_node);
                        o.pack(input.num_threads_per_proc);
                        return o;
                    }
                };

                template<>
                struct object_with_zone<ResourceAllocation> {
                    void operator()(mv1::object::with_zone &o, ResourceAllocation const &input) const {
                        o.type = type::ARRAY;
                        o.via.array.size = 3;
                        o.via.array.ptr = static_cast<clmdep_msgpack::object *>(o.zone.allocate_align(
                                sizeof(mv1::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(mv1::object)));
                        o.via.array.ptr[0] = mv1::object(input.num_nodes_, o.zone);
                        o.via.array.ptr[1] = mv1::object(input.num_procs_per_node, o.zone);
                        o.via.array.ptr[2] = mv1::object(input.num_threads_per_proc, o.zone);
                    }
                };
            }  // namespace adaptor
    }
}  // namespace clmdep_msgpack



std::ostream &operator<<(std::ostream &os, Data &data);

#endif //RHEA_DATA_STRUCTURES_H
