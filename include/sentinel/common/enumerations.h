//
// Created by mani on 9/14/2020.
//

#ifndef SENTINEL_COMMON_ENUMERATIONS_H
#define SENTINEL_COMMON_ENUMERATIONS_H
#include <rpc/msgpack.hpp>
enum class OperationType{
    PUBLISH=0,
    SUBSCRIBE=1
};
MSGPACK_ADD_ENUM(OperationType);

#endif //SENTINEL_COMMON_ENUMERATIONS_H
