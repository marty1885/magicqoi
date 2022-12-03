cmake_minimum_required(VERSION 3.15)
project(magicqoi)

add_library(magicqoi magicqoi/magicqoi.c)
include_directories(magicqoi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library shared by examples and benchmarks
add_subdirectory(common)

option(MAGICQOI_BUILD_EXAMPLES "Build examples" ON)
if(MAGICQOI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(MAGICQOI_BUILD_BENCHMARK "Build benchmark" ON)
if(MAGICQOI_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

option(MAGICQOI_BUILD_TESTS "Build tests" ON)
if(MAGICQOI_BUILD_TESTS)
    add_subdirectory(tests)
endif()