cmake_minimum_required(VERSION 3.10)
project(scs_io_common)
set(INSTALL_DIR $ENV{INSTALL_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${INCLUDE_DIRS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${INCLUDE_DIRS}")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/scs_io_common/include)

SET(JOB_SRC ${CMAKE_SOURCE_DIR}/sentinel/test/unit/class_loader/rhea_job.cpp)
add_library(job SHARED ${JOB_SRC})
add_dependencies(job scs_io_common)
target_link_libraries(job -ldl)

# Executables
add_executable(class_loader_test ../../../sentinel/test/unit/class_loader/test.cpp)
add_dependencies(class_loader_test scs_io_common)
target_link_libraries(class_loader_test scs_io_common -ldl)

add_executable(worker_manager_client_test_exec worker_manager/test.cpp)
add_dependencies(worker_manager_client_test_exec scs_io_common sentinel)
target_link_libraries(worker_manager_client_test_exec scs_io_common sentinel)

add_test(NAME worker_manager_server_test COMMAND ${CMAKE_BINARY_DIR}/sentinel/sentinel_worker_manager)
add_test(NAME worker_manager_client_test COMMAND ${CMAKE_CURRENT_BINARY_DIR}/worker_manager_client_test_exec)