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