cmake_minimum_required(VERSION 3.14)
project(SubmodulesProject VERSION 1.0.0 LANGUAGES C CXX)

option(BUILD_TESTING "Build and run tests" ON)
if(BUILD_TESTING)
    enable_testing()
    include(CTest)
endif()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(bench)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()