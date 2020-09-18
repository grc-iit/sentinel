cmake_minimum_required(VERSION 3.10)
project(scs_io_common)
set(INSTALL_DIR $ENV{INSTALL_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${INCLUDE_DIRS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_DIRS}")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/scs_io_common/include)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/job_so)
set(test_files  job_1 job_2)
foreach (test_file ${test_files})
    set(job ${test_file})
    set(JOB_SRC ${CMAKE_SOURCE_DIR}/sentinel/test/unit/class_loader/${test_file}/test.cpp)
    add_library(${job} SHARED ${JOB_SRC})
    add_dependencies(${job} scs_io_common)
    target_link_libraries(${job} -ldl)
endforeach ()

# Executables
add_executable(class_loader_test ../../../sentinel/test/unit/class_loader/test.cpp)
add_dependencies(class_loader_test scs_io_common)
target_link_libraries(class_loader_test scs_io_common -ldl)

add_executable(worker_manager_client_test_exec worker_manager/test.cpp)
add_dependencies(worker_manager_client_test_exec scs_io_common sentinel)
target_link_libraries(worker_manager_client_test_exec scs_io_common sentinel)

add_executable(thread_pool_exec worker_manager/thread_pool.cpp ${CMAKE_SOURCE_DIR}/sentinel/src/sentinel/worker_manager/server.cpp)
add_dependencies(thread_pool_exec scs_io_common sentinel)
target_link_libraries(thread_pool_exec scs_io_common sentinel)

add_test(NAME worker_manager_server_test COMMAND ${CMAKE_BINARY_DIR}/sentinel/sentinel_worker_manager ${CMAKE_SOURCE_DIR}/sentinel/conf/sentinel_conf.json)
set_property(TEST worker_manager_server_test APPEND PROPERTY ENVIRONMENT "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
set_property(TEST worker_manager_server_test APPEND PROPERTY ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

add_test(NAME worker_manager_client_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/worker_manager_client_test_exec ${CMAKE_SOURCE_DIR}/sentinel/conf/sentinel_conf.json)
set_property(TEST worker_manager_client_test APPEND PROPERTY ENVIRONMENT "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
set_property(TEST worker_manager_client_test APPEND PROPERTY ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

add_test(NAME thread_pool_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/thread_pool_exec)

add_executable(mpi_comm_spawn_simple_test jobmanager/com_spawn_test.cpp)
add_executable(mpi_comm_spawn_executable_test jobmanager/executable.cpp)
add_executable(jobmanager_test jobmanager/jobmanager_test.cpp)
add_executable(jobmanager_client_test jobmanager/jobmanager_client_test.cpp)
add_dependencies(mpi_comm_spawn_simple_test mpi_comm_spawn_executable_test)
target_link_libraries(mpi_comm_spawn_simple_test -lmpi)
target_link_libraries(mpi_comm_spawn_executable_test -lmpi)
target_link_libraries(jobmanager_test -lmpi)
target_link_libraries(jobmanager_client_test -lmpi)

#add_test(NAME mpi_comm_spawn_simple_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/mpi_comm_spawn_simple_test)
#set_property(TEST mpi_comm_spawn_simple_test APPEND PROPERTY ENVIRONMENT "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
#set_property(TEST mpi_comm_spawn_simple_test APPEND PROPERTY ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")
