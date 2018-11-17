cmake_minimum_required(VERSION 3.10)
project(concurrency_example)

set(CMAKE_CXX_STANDARD 17)
set(CONCURRENCY_HPP ${CMAKE_SOURCE_DIR}/concurrency)

add_executable(concurrency_example example.cpp)

target_link_libraries(concurrency_example stdc++fs)
include_directories(concurrency_example PRIVATE ${CONCURRENCY_HPP})
