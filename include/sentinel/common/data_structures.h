//
// Created by mani on 9/14/2020.
//

#ifndef SENTINEL_COMMON_DATA_STRUCTURES_H
#define SENTINEL_COMMON_DATA_STRUCTURES_H


#include <basket/common/data_structures.h>
#include <cstdint>
#include <rpc/msgpack.hpp>

#include <basket.h>
#include <common/data_structure.h>
#include <sentinel/common/enumerations.h>
#include <vector>

typedef struct Event: public Data{

    OperationType type_;
    /*Define the default, copy and move constructor*/
    Event(): Data(),type_(){}

    Event(const Event &other): Data(other),type_(other.type_){}
    Event(Event &other): Data(other),type_(other.type_){}
    /*Define Assignment Operator*/
    Event &operator=(const Event &other){
        Data::operator=(other);
        type_ = other.type_;
        return *this;
    }
}Event;

template<typename E>
struct Task{
public:
    uint32_t job_id_;
    uint32_t id_;
    TaskType type_;
    std::vector<std::shared_ptr<Task>> links;

    Task(TaskType type=TaskType::SOURCE):links(),type_(type){}
    Task(const Task &other):id_(other.id_),job_id_(other.job_id_),links(other.links),type_(other.type_){};
    Task(Task &&other):id_(other.id_),job_id_(other.job_id_),links(other.links),type_(other.type_){};
    Task &operator=(const Task &other){
        id_ = other.id_;
        links = other.links;
        job_id_=other.job_id_;
        type_=other.type_;
        return *this;
    }

    bool EmitCallback(std::function<bool(uint32_t, uint32_t, Event &)> &func){
        emit=func;
        return true;
    }
protected:

    std::function<bool(uint32_t, uint32_t, Event &)> emit;

};
template<typename E, typename std::enable_if<std::is_base_of<Event, E>::value>::type * = nullptr>
struct SourceTask: public Task<E>{
    SourceTask():Task<E>(TaskType::SOURCE){}
    SourceTask(Task<E> t):Task<E>(t){}
    virtual E Execute(E &event){
        bool status = true;
        status = status && Initialize(event);
        E output;
        if(status) output = Run(event);
        status = status && Finalize(event);
        return output;
    }
protected:
    virtual bool Initialize(E &event) = 0;
    virtual E Run(E &event) = 0;
    virtual bool Finalize(E &event) = 0;
};
template<typename E,  typename std::enable_if<std::is_base_of<Event, E>::value>::type * = nullptr>
struct KeyByTask: public Task<E>{
    KeyByTask():Task<E>(TaskType::KEYBY){}
    virtual size_t Execute(E &event){
        bool status = true;
        status = status && Initialize(event);
        size_t output;
        if(status) output = Run(event);
        status = status && Finalize(event);
        return output;
    }
protected:
    virtual bool Initialize(E &event) = 0;
    virtual size_t Run(E &event) = 0;
    virtual bool Finalize(E &event) = 0;
};
template<typename E, typename std::enable_if<std::is_base_of<Event, E>::value>::type * = nullptr>
struct SinkTask: public Task<E>{
    SinkTask():Task<E>(TaskType::SINK){}
    virtual void Execute(E &event){
        bool status = true;
        status = status && Initialize(event);
        if(status) Run(event);
        status = status && Finalize(event);
    }
protected:
    virtual bool Initialize(E &event) = 0;
    virtual void Run(E &event) = 0;
    virtual bool Finalize(E &event) = 0;
};

template<typename E, typename std::enable_if<std::is_base_of<Event, E>::value>::type * = nullptr>
struct Job{
protected:
    std::shared_ptr<Task<E>> source_;
    std::shared_ptr<Task<E>> FindTask(uint32_t task_id_, std::shared_ptr<Task<E>> &currentTask){
        if(task_id_ == currentTask->id_) return currentTask;
        else for(auto link: currentTask->links ) {
            auto task = FindTask(task_id_, link);
            if(task != NULL) return task;
        }
        return NULL;
    }
public:
    uint32_t job_id_;
    Job(uint32_t job_id): source_(),job_id_(job_id){}
    Job(const Job &other): source_(other.source_),job_id_(other.job_id_) {}
    Job(Job &other): source_(other.source_),job_id_(other.job_id_) {}
    /*Define Assignment Operator*/
    Job &operator=(const Job &other){
        source_ = other.source_;
        job_id_=other.job_id_;
        return *this;
    }

    std::shared_ptr<Task<E>> GetTask(uint32_t task_id_=0){
        //Task id = 0 is the collector
        if(task_id_==0) return source_;
        else return FindTask(task_id_, source_);
    }

    virtual void CreateDAG()=0;

};


typedef struct ResourceAllocation {
    uint16_t job_id_;
    uint16_t num_nodes_;
    uint16_t num_procs_per_node;
    uint16_t num_threads_per_proc;

    ResourceAllocation(uint16_t job_id, uint16_t num_nodes, uint16_t num_procs_per_node, uint16_t num_threads_per_proc): job_id_(job_id),num_nodes_(num_nodes), num_procs_per_node(num_procs_per_node), num_threads_per_proc(num_threads_per_proc){}

    /*Define the default, copy and move constructor*/
    ResourceAllocation(): job_id_(0),num_nodes_(0), num_procs_per_node(0), num_threads_per_proc(){}
    ResourceAllocation(const ResourceAllocation &other): job_id_(other.job_id_),num_nodes_(other.num_nodes_), num_procs_per_node(other.num_procs_per_node), num_threads_per_proc(other.num_threads_per_proc){}
    ResourceAllocation(ResourceAllocation &other): job_id_(other.job_id_) ,num_nodes_(other.num_nodes_), num_procs_per_node(other.num_procs_per_node), num_threads_per_proc(other.num_threads_per_proc){}

    /*Define Assignment Operator*/
    ResourceAllocation &operator=(const ResourceAllocation &other){
        num_nodes_ = other.num_nodes_;
        num_procs_per_node = other.num_procs_per_node;
        num_threads_per_proc = other.num_threads_per_proc;
        job_id_ = other.job_id_;
        return *this;
    }
} ResourceAllocation;

typedef struct WorkerManagerStats {

    double thrpt_kops_;
    uint32_t num_tasks_exec_;
    uint32_t num_tasks_queued_;
    WorkerManagerStats():thrpt_kops_(0),num_tasks_exec_(0),num_tasks_queued_(0){}
    WorkerManagerStats(double epoch_time, int num_tasks_assigned, int num_tasks_queued) {
        num_tasks_exec_ = num_tasks_assigned - num_tasks_queued;
        num_tasks_queued_ = num_tasks_queued;
        thrpt_kops_ = num_tasks_exec_ / epoch_time;
    }
    /*Define the default, copy and move constructor*/
    WorkerManagerStats(const WorkerManagerStats &other): thrpt_kops_(other.thrpt_kops_), num_tasks_exec_(other.num_tasks_exec_), num_tasks_queued_(other.num_tasks_queued_) {}
    WorkerManagerStats(WorkerManagerStats &other):  thrpt_kops_(other.thrpt_kops_), num_tasks_exec_(other.num_tasks_exec_), num_tasks_queued_(other.num_tasks_queued_) {}
    /*Define Assignment Operator*/
    WorkerManagerStats &operator=(const WorkerManagerStats &other)= default;

    bool operator<(const WorkerManagerStats &other) const{
        return (num_tasks_queued_ < other.num_tasks_queued_);
    }
} WorkerManagerStats;
namespace clmdep_msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
        namespace mv1 = clmdep_msgpack::v1;

        namespace adaptor {
            template<>
            struct convert<Event> {
                mv1::object const &operator()(mv1::object const &o, Event &input) const {
                    input.id_ = o.via.array.ptr[0].as<CharStruct>();
                    input.position_ = o.via.array.ptr[1].as<size_t>();
                    auto data = o.via.array.ptr[2].as<std::string>();
                    input.data_size_ = o.via.array.ptr[3].as<size_t>();
                    if (!data.empty()) {
                        input.buffer_ = static_cast<char *>(malloc(input.data_size_));
                        memcpy(input.buffer_, data.data(), input.data_size_);
                    }
                    input.storage_index_ = o.via.array.ptr[4].as<uint16_t>();
                    return o;
                }
            };

            template<>
            struct pack<Event> {
                template<typename Stream>
                packer<Stream> &operator()(mv1::packer<Stream> &o, Event const &input) const {
                    o.pack_array(5);
                    o.pack(input.id_);
                    o.pack(input.position_);
                    if (input.buffer_ == NULL) o.pack(std::string());
                    else {
                        o.pack(std::string(input.buffer_, input.data_size_));
                    }
                    o.pack(input.data_size_);
                    o.pack(input.storage_index_);
                    return o;
                }
            };

            template<>
            struct object_with_zone<Event> {
                void operator()(mv1::object::with_zone &o, Event const &input) const {
                    o.type = type::ARRAY;
                    o.via.array.size = 5;
                    o.via.array.ptr = static_cast<clmdep_msgpack::object *>(o.zone.allocate_align(
                            sizeof(mv1::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(mv1::object)));
                    o.via.array.ptr[0] = mv1::object(input.id_, o.zone);
                    o.via.array.ptr[1] = mv1::object(input.position_, o.zone);
                    if (input.buffer_ == NULL) o.via.array.ptr[2] = mv1::object(std::string(), o.zone);
                    else {
                        o.via.array.ptr[2] = mv1::object(std::string(input.buffer_, input.data_size_), o.zone);
                        free(input.buffer_);
                    }
                    o.via.array.ptr[3] = mv1::object(input.data_size_, o.zone);
                    o.via.array.ptr[4] = mv1::object(input.storage_index_, o.zone);
                }
            };

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
                packer<Stream> &operator()(mv1::packer<Stream> &o, WorkerManagerStats const &input) const {
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
                packer<Stream> &operator()(mv1::packer<Stream> &o, ResourceAllocation const &input) const {
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
        }
    }
};

std::ostream &operator<<(std::ostream &os, Data &data);

std::ostream &operator<<(std::ostream &os, Event &event);

#endif //SENTINEL_COMMON_DATA_STRUCTURES_H
