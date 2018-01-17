add_subdirectory(third_party)

add_executable(unit_test
        Main.cpp)

target_link_libraries(unit_test
        detector
        libgtest
        libgmock)
