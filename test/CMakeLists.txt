cmake_minimum_required(VERSION 3.10)
project(catch_test)

set(CMAKE_CXX_STANDARD 17)

file(GLOB test_files 
        "impl/*.cpp"
        "impl/lockfree/*.cpp"
)

add_executable(catch_test main.cpp ${test_files})
target_include_directories(catch_test PRIVATE ../concurrency/impl)

add_subdirectory(../external/Catch2 external)
target_link_libraries(catch_test Catch2::Catch2)

if(UNIX)
    find_package(Threads REQUIRE)
    target_link_libraries(catch_test Threads::Threads)
endif(UNIX)
