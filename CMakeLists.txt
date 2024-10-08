cmake_minimum_required(VERSION 3.25)
project(LENet)

set(CMAKE_CXX_STANDARD 20)

add_executable(LENet main.cpp BufferPool.cpp BufferPool.hpp NetConnection.hpp)
