cmake_minimum_required(VERSION 3.8)
project(compromutator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-long-long -pedantic -O0 -g")

enable_testing()

add_subdirectory(src)
add_subdirectory(test)

add_executable(compromutator src/Main.cpp)
target_link_libraries(compromutator detector)
