cmake_minimum_required(VERSION 3.10)
project(sentinel VERSION 1.0.0 DESCRIPTION "")
set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(include)

set(SENTINEL_COMMON )
set(SENTINEL_LIB ${SENTINEL_COMMON} src/sentinel/lib/client.cpp include/sentinel/client.h src/sentinel/job_manager/client.cpp include/sentinel/job_manager/client.h)
add_library(sentinel SHARED ${SENTINEL_LIB})


set(SENTINEL_JOB_MANAGER ${SENTINEL_COMMON} src/sentinel/job_manager/server.cpp src/sentinel/job_manager/server.h src/sentinel/job_manager/main.cpp)
add_executable(sentinel_job_manager ${SENTINEL_JOB_MANAGER})

set(SENTINEL_WORKER_MANAGER ${SENTINEL_COMMON} src/sentinel/worker_manager/server.cpp src/sentinel/worker_manager/server.h src/sentinel/worker_manager/main.cpp src/sentinel/worker_manager/client.cpp include/sentinel/worker_manager/client.h)
add_executable(sentinel_worker_manager ${SENTINEL_WORKER_MANAGER})

set(SENTINEL_MONITOR_LIB ${SENTINEL_COMMON} src/sentinel/monitor/client.cpp include/sentinel/monitor/client.h)
add_library(sentinel_monitor SHARED ${SENTINEL_MONITOR_LIB})

set(SENTINEL_MONITOR_SERVER ${SENTINEL_COMMON})
add_executable(sentinel_monitor_server ${SENTINEL_MONITOR_SERVER} src/sentinel/monitor/server.cpp src/sentinel/monitor/server.h src/sentinel/monitor/main.cpp)