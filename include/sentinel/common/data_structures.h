//
// Created by mani on 9/14/2020.
//

#ifndef RHEA_DATA_STRUCTURES_H
#define RHEA_DATA_STRUCTURES_H


#include <basket/common/data_structures.h>
#include <rpc/msgpack.hpp>

#include <basket.h>

typedef struct Data {

    CharStruct id_; // for file io, the "id_" is the filename; for object store io, the "id_" is the key.
    size_t position_; // read/write start position
    char* buffer_;  // data content
    size_t data_size_;
    uint16_t storage_index_;

    /*Define the default, copy and move constructor*/
    Data(): id_(), position_(0), buffer_(NULL), storage_index_(),data_size_(){}
    Data(const Data &other): id_(other.id_), position_(other.position_), buffer_(other.buffer_),
                             storage_index_(other.storage_index_),data_size_(other.data_size_) {}
    Data(Data &other): id_(other.id_), position_(other.position_), buffer_(other.buffer_),
                       storage_index_(other.storage_index_),data_size_(other.data_size_) {}

    /*Define Assignment Operator*/
    Data &operator=(const Data &other){
        id_ = other.id_;
        position_ = other.position_;
        buffer_ = other.buffer_;
        data_size_ = other.data_size_;
        storage_index_ = other.storage_index_;
        return *this;
    }
} Data;


typedef struct ResourceAllocation {

    CharStruct id_; // for file io, the "id_" is the filename; for object store io, the "id_" is the key.
    size_t position_; // read/write start position
    char* buffer_;  // data content
    size_t data_size_;
    uint16_t storage_index_;

    /*Define the default, copy and move constructor*/
    ResourceAllocation(): id_(), position_(0), buffer_(NULL), storage_index_(),data_size_(){}
    ResourceAllocation(const Data &other): id_(other.id_), position_(other.position_), buffer_(other.buffer_),
                             storage_index_(other.storage_index_),data_size_(other.data_size_) {}
    ResourceAllocation(ResourceAllocation &other): id_(other.id_), position_(other.position_), buffer_(other.buffer_),
                       storage_index_(other.storage_index_),data_size_(other.data_size_) {}

    /*Define Assignment Operator*/
    ResourceAllocation &operator=(const ResourceAllocation &other){
        id_ = other.id_;
        position_ = other.position_;
        buffer_ = other.buffer_;
        data_size_ = other.data_size_;
        storage_index_ = other.storage_index_;
        return *this;
    }
} ResourceAllocation;

typedef struct WorkerManagerStats {

   double thrpt_kops;
    int num_tasks_exec_;
    int num_tasks_queued_;

    WorkerManagerStats(double epoch_time, int num_tasks_assigned, int num_tasks_queued) {
        num_tasks_queued_ = num_tasks_queued;
        num_tasks_exec_ = num_tasks_assigned - num_tasks_queued_;
        thrpt_kops = num_tasks_exec_ / thrpt_kops;
    }
} WorkerManagerStats;


namespace clmdep_msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
        namespace adaptor {
            namespace mv1 = clmdep_msgpack::v1;
            template<>
            struct convert<Data> {
                mv1::object const &operator()(mv1::object const &o, Data &input) const {
                    input.id_ = o.via.array.ptr[0].as<CharStruct>();
                    input.position_ = o.via.array.ptr[1].as<size_t>();
                    auto data = o.via.array.ptr[2].as<std::string>();
                    input.data_size_ = o.via.array.ptr[3].as<size_t>();
                    if(!data.empty()) {
                        input.buffer_= static_cast<char *>(malloc(input.data_size_));
                        memcpy(input.buffer_ , data.data(),input.data_size_);
                    }
                    input.storage_index_ = o.via.array.ptr[4].as<uint16_t>();
                    return o;
                }
            };

            template<>
            struct pack<Data> {
                template<typename Stream>
                packer <Stream> &operator()(mv1::packer <Stream> &o, Data const &input) const {
                    o.pack_array(5);
                    o.pack(input.id_);
                    o.pack(input.position_);
                    if(input.buffer_ == NULL) o.pack(std::string());
                    else {
                        o.pack(std::string(input.buffer_,input.data_size_));
                    }
                    o.pack(input.data_size_);
                    o.pack(input.storage_index_);
                    return o;
                }
            };

            template<>
            struct object_with_zone<Data> {
                void operator()(mv1::object::with_zone &o, Data const &input) const {
                    o.type = type::ARRAY;
                    o.via.array.size = 5;
                    o.via.array.ptr = static_cast<clmdep_msgpack::object *>(o.zone.allocate_align(
                            sizeof(mv1::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(mv1::object)));
                    o.via.array.ptr[0] = mv1::object(input.id_, o.zone);
                    o.via.array.ptr[1] = mv1::object(input.position_, o.zone);
                    if(input.buffer_ == NULL) o.via.array.ptr[2] = mv1::object(std::string(), o.zone);
                    else {
                        o.via.array.ptr[2] = mv1::object(std::string(input.buffer_,input.data_size_), o.zone);
                        free(input.buffer_);
                    }
                    o.via.array.ptr[3] = mv1::object(input.data_size_, o.zone);
                    o.via.array.ptr[4] = mv1::object(input.storage_index_, o.zone);
                }
            };
        }  // namespace adaptor
    }
}  // namespace clmdep_msgpack
std::ostream &operator<<(std::ostream &os, Data &data);

#endif //RHEA_DATA_STRUCTURES_H
