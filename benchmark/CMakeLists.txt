find_package(benchmark REQUIRED)

add_executable(decode_benchmark decode_benchmark.cpp mgqoi_baseline.c)
target_link_libraries(decode_benchmark PRIVATE benchmark::benchmark magicqoi)

add_executable(decode_set_benchmark decode_set_benchmark.cpp)
target_link_libraries(decode_set_benchmark PRIVATE benchmark::benchmark magicqoi)