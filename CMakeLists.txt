cmake_minimum_required(VERSION 3.25)
project(LENet)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE srcs
    "Source/*.h"
    "Source/*.hpp"
    "Source/*.cpp"
)

add_executable(${CMAKE_PROJECT_NAME} ${srcs})
