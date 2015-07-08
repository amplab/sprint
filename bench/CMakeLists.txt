cmake_minimum_required(VERSION 2.8)
project(pull-star-bench CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(INCLUDE include/)
set(EXTERNAL_INCLUDE ../external/ds-lib/bench/include)
set(SOURCE_FILES
    src/regex_bench.cc
    include/regex_bench.h)
include_directories(${INCLUDE} ${EXTERNAL_INCLUDE})
add_executable(rxbench ${SOURCE_FILES})
target_link_libraries(rxbench pullstar)