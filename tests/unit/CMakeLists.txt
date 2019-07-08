find_package(Threads REQUIRED)

set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# Download and unpack googletest at configure time
message(${CMAKE_SOURCE_DIR}/third_party/googletest.cmake)
configure_file(
    ${CMAKE_SOURCE_DIR}/third_party/googletest.cmake
    ${CMAKE_BINARY_DIR}/third_party/googletest-download/CMakeLists.txt
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third_party/googletest-download
)
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/third_party/googletest-download
)
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(
    ${CMAKE_BINARY_DIR}/third_party/googletest-src
    ${CMAKE_BINARY_DIR}/third_party/googletest-build
    EXCLUDE_FROM_ALL
)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

add_executable(unit_test
    MainTest.cpp
    HeaderSpaceTest.cpp
    ExampleNetwork.hpp
    NetworkTest.cpp
    DependencyGraphTest.cpp
    ExampleDependencyGraph.hpp
    FlowPredictorTest.cpp
    ParserTest.cpp
)

target_link_libraries(unit_test
    detector
    gtest_main
)
