add_subdirectory(third_party)

add_executable(unit_test
        MainTest.cpp
        HeaderSpaceTest.cpp
        ExampleNetwork.hpp
        NetworkTest.cpp)

target_link_libraries(unit_test
        detector
        libgtest
        libgmock)
