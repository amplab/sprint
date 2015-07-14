cmake_minimum_required(VERSION 2.8)
project(pull-star-thrift CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-modules)

find_package(Thrift REQUIRED)

set(AGGREGATOR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/template/aggregator.thrift)
set(SHARD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/template/shard.thrift)
set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(MAKE_DIRECTORY ${INCLUDE_DIR})

thrift_gen_cpp(${AGGREGATOR_FILE} ${SOURCES_DIR} ${INCLUDE_DIR} AGGREGATOR_SOURCES)
thrift_gen_cpp(${SHARD_FILE} ${SOURCES_DIR} ${INCLUDE_DIR} SHARD_SOURCES)

set(AGGREGATOR_SOURCES ${AGGREGATOR_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/aggregator_server.cc" ${SHARD_SOURCES})
set(SHARD_SOURCES ${SHARD_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/shard_server.cc")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

set(EXTERNAL_INCLUDE ../external/ds-lib/bench/include ../external/ds-lib/external/cst_v_1_1)
include_directories(${EXTERNAL_INCLUDE} ${INCLUDE_DIR})
add_executable(rxshard ${SHARD_SOURCES})
add_executable(rxaggregator ${AGGREGATOR_SOURCES})
target_link_libraries(rxshard pullstar)
target_link_libraries(rxshard ${THRIFT_LIBRARIES})
target_link_libraries(rxaggregator pullstar)
target_link_libraries(rxaggregator ${THRIFT_LIBRARIES})
