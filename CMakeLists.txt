cmake_minimum_required(VERSION 2.8)
project(pull-star CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set(INCLUDE include)
set(EXTERNAL_INCLUDE external/ds-lib/include)
file(GLOB_RECURSE SOURCE_FILES src/*.cc)
include_directories(${INCLUDE} ${EXTERNAL_INCLUDE})
add_library(pullstar STATIC ${SOURCE_FILES})
set_target_properties(pullstar PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(pullstar ds)
target_link_libraries(pullstar cst)

add_subdirectory(external/ds-lib)
add_subdirectory(bench)
add_subdirectory(shell)
