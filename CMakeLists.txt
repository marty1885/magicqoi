cmake_minimum_required(VERSION 3.15)
project(magicqoi)

add_library(magicqoi magicqoi/magicqoi.c)
include_directories(magicqoi)

option(MAGICQOI_BUILD_EXAMPLES "Build examples" ON)
if(MAGICQOI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(MAGICQOI_BUILD_BENCHMARK "Build BENCHMARK" ON)
if(MAGICQOI_BUILD_BENCHMARK)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    add_subdirectory(benchmark)
endif()
