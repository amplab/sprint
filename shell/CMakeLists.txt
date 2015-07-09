cmake_minimum_required(VERSION 2.8)
project(pull-star-shell CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(EXTERNAL_INCLUDE ../external/ds-lib/bench/include)
file(GLOB SOURCE_FILES src/*.cc)
include_directories(${EXTERNAL_INCLUDE})
add_executable(rxshell ${SOURCE_FILES})
target_link_libraries(rxshell pullstar)
