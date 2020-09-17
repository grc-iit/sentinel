//
// Created by neeraj on 9/16/20.
//

#include <sentinel/common/data_structures.h>


std::ostream &operator<<(std::ostream &os, Data &data){
    return os << "{id_:" << data.id_ << ","
              << "data_size_:" << data.data_size_ << ","
              << "position_:" << data.position_ << ","
              << "storage_index_:" << data.storage_index_ << "}";
}

