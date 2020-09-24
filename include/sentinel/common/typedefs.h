//
// Created by hdevarajan on 9/23/20.
//

#ifndef SENTINEL_TYPEDEFS_H
#define SENTINEL_TYPEDEFS_H

#include <cstdint>
#include <basket/common/data_structures.h>

typedef uint32_t JobId, WorkerManagerId, TaskId;
typedef uint16_t ThreadId, StartThreadId, EndThreadId;
typedef CharStruct NodeName;
typedef std::size_t HashValue;
#endif //SENTINEL_TYPEDEFS_H
