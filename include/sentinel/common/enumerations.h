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

enum class TaskType{
    SOURCE=0,
    SINK=1,
    KEYBY=2
};

MSGPACK_ADD_ENUM(TaskType);

enum class TaskStatus{
    NONE=0,
    QUEUED=1,
    IN_PROGRESS=2,
    DONE=3
};
MSGPACK_ADD_ENUM(TaskStatus);
#endif //SENTINEL_COMMON_ENUMERATIONS_H
