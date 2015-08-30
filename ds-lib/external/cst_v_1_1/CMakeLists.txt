cmake_minimum_required(VERSION 2.8)
project(cst CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

file(GLOB SOURCE_FILES *.cpp)
add_library(cst STATIC ${SOURCE_FILES})
