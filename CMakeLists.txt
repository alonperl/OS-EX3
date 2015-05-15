cmake_minimum_required(VERSION 3.2)
project(ex3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_custom_target(mytest COMMAND make -C ${ex3_SOURCE_DIR}
                         CLION_EXE_DIR=${PROJECT_BINARY_DIR})