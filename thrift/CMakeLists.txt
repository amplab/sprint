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

# prefer the thrift version supplied in THRIFT_HOME (cmake -DTHRIFT_HOME then environment)
find_path(THRIFT_INCLUDE_DIR
    NAMES
        thrift/Thrift.h
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        include
)

# prefer the thrift version supplied in THRIFT_HOME
find_library(THRIFT_LIBRARIES
    NAMES
        thrift libthrift
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        lib lib64
)

find_program(THRIFT_COMPILER
    NAMES
        thrift
    HINTS
        ${THRIFT_HOME}
        ENV THRIFT_HOME
        /usr/local
        /opt/local
    PATH_SUFFIXES
        bin bin64
)

if (THRIFT_COMPILER)
    exec_program(${THRIFT_COMPILER}
        ARGS -version OUTPUT_VARIABLE __thrift_OUT RETURN_VALUE THRIFT_RETURN)
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+-[a-z]+$" THRIFT_VERSION_STRING ${__thrift_OUT})

    # define utility function to generate cpp files
    function(thrift_gen_cpp thrift_file dst_src dst_inc SRC_LIST)
        set(_res)
        set(_res_inc_path)
        if(EXISTS ${thrift_file})
            get_filename_component(_target_dir ${thrift_file} NAME_WE)
            message(STATUS "Generating thrift C++ files for ${_target_dir}")
            if(NOT EXISTS ${CMAKE_BINARY_DIR}/${_target_dir})
                file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${_target_dir})
            endif()
            execute_process(COMMAND ${THRIFT_COMPILER} -o "${CMAKE_BINARY_DIR}/${_target_dir}" --gen cpp ${thrift_file})
            file(GLOB_RECURSE __result_src "${CMAKE_BINARY_DIR}/${_target_dir}/*.cpp")
            file(GLOB_RECURSE __result_hdr "${CMAKE_BINARY_DIR}/${_target_dir}/*.h")
        else()
            message(SEND_ERROR "thrift_gen_cpp: File ${thrift_file} does not exist")
        endif()
        set(${THRIFT_CPP_FILES} "${__result_src}" PARENT_SCOPE)
        set(${THRIFT_H_FILES} "${__result_hdr}" PARENT_SCOPE)
        foreach(src_file ${__result_src})
            get_filename_component(filename ${src_file} NAME)
            if(filename MATCHES "(.*)\\.skeleton\\.(.*)")
                message(STATUS "Ignoring ${filename}")
            else()
                file(COPY ${src_file} DESTINATION ${dst_src})
                list(APPEND _sources "${dst_src}/${filename}")
            endif()
        endforeach(src_file)
        foreach(hdr_file ${__result_hdr})
            file(COPY ${hdr_file} DESTINATION ${dst_inc})
        endforeach(hdr_file)
        set(${SRC_LIST} "${_sources}" PARENT_SCOPE)
    endfunction()
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(THRIFT DEFAULT_MSG THRIFT_LIBRARIES THRIFT_INCLUDE_DIR THRIFT_COMPILER)
mark_as_advanced(THRIFT_LIBRARIES THRIFT_INCLUDE_DIR THRIFT_COMPILER THRIFT_VERSION_STRING)

set(AGGREGATOR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/template/aggregator.thrift)
set(SHARD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/template/shard.thrift)
set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
