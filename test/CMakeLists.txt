#add_subdirectory(third_party)

find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)

set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# Download and install GoogleTest
ExternalProject_Add(
        gtest
        #URL https://github.com/google/googletest/archive/master.zip
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG "master"
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/../gtest
        # Disable install step
        INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include"
        "${source_dir}/googlemock/include")

add_executable(unit_test
        MainTest.cpp
        HeaderSpaceTest.cpp
        ExampleNetwork.hpp
        NetworkTest.cpp
        DependencyGraphTest.cpp
        ExampleDependencyGraph.hpp
        FlowPredictorTest.cpp)

target_link_libraries(unit_test
        detector
        libgtest
        libgmock)
