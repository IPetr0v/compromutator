cmake_minimum_required(VERSION 3.2)
enable_testing()
project(compromutator)

include(third_party/libfluid.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Werror -Wno-long-long -pedantic -O0 -g")

add_subdirectory(src)
add_subdirectory(tests/unit)

add_executable(compromutator src/Main.cpp)
target_link_libraries(compromutator detector fluid_msg cxxopts)
