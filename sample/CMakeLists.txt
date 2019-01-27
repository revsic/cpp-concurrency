cmake_minimum_required(VERSION 3.10)
project(sample)

set(CMAKE_CXX_STANDARD 17)

add_executable(dir_size dir_size.cpp)
add_executable(tick tick.cpp)

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(dir_size Threads::Threads)
    target_link_libraries(tick Threads::Threads)

    target_link_libraries(dir_size stdc++fs)
endif(UNIX)
