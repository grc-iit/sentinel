cmake_minimum_required(VERSION 3.10)
project(sentinel VERSION 1.0.0 DESCRIPTION "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -g -DBASKET_ENABLE_RPCLIB")

include_directories(src)
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/scs_io_common/include)

set(LIBS -lpthread -lbasket -lrpc  -lboost_system -lboost_filesystem -lmpi -ldl -L${CMAKE_BINARY_DIR}/scs_io_common -lscs_io_common)

set(SENTINEL_COMMON include/sentinel/common/configuration_manager.h include/sentinel/common/data_structures.h
        include/sentinel/common/enumerations.h include/sentinel/common/error_codes.h include/sentinel/common/error_definition.h
        include/sentinel/common/debug.h include/sentinel/common/daemon.h)

set(SENTINEL_LIB ${SENTINEL_COMMON} src/sentinel/job_manager/client.cpp include/sentinel/job_manager/client.h src/sentinel/worker_manager/client.cpp include/sentinel/worker_manager/client.h src/sentinel/monitor/client.cpp include/sentinel/monitor/client.h)
add_library(sentinel SHARED ${SENTINEL_LIB})

target_link_libraries(sentinel ${LIBS})

set(SENTINEL_JOB_MANAGER ${SENTINEL_COMMON} src/sentinel/job_manager/server.cpp include/sentinel/job_manager/Server.h src/sentinel/job_manager/main.cpp)
add_executable(sentinel_job_manager ${SENTINEL_JOB_MANAGER})
add_dependencies(sentinel_job_manager sentinel)
target_link_libraries(sentinel_job_manager ${LIBS})

set(SENTINEL_WORKER_MANAGER ${SENTINEL_COMMON} src/sentinel/worker_manager/main.cpp src/sentinel/worker_manager/server.cpp src/sentinel/worker_manager/server.h)
add_executable(sentinel_worker_manager ${SENTINEL_WORKER_MANAGER})
add_dependencies(sentinel_worker_manager sentinel)
target_link_libraries(sentinel_worker_manager ${LIBS})

set(SENTINEL_MONITOR_LIB ${SENTINEL_COMMON} src/sentinel/monitor/client.cpp include/sentinel/monitor/client.h)
add_library(sentinel_monitor SHARED ${SENTINEL_MONITOR_LIB})
add_dependencies(sentinel_monitor sentinel)
target_link_libraries(sentinel_monitor ${LIBS})

set(SENTINEL_MONITOR_SERVER ${SENTINEL_COMMON})
add_executable(sentinel_monitor_server ${SENTINEL_MONITOR_SERVER} src/sentinel/monitor/server.cpp src/sentinel/monitor/server.h src/sentinel/monitor/main.cpp)
target_link_libraries(sentinel_monitor_server ${LIBS})
add_dependencies(sentinel_monitor_server sentinel)


# Enable tests
option(BUILD_TEST "Build the unit tests" ON)
if(BUILD_TEST)
    enable_testing()
    include(CTest)
    add_subdirectory(test/unit)
endif()