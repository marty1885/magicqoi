add_executable(unittest unittest.cpp)

find_package(Catch2 3 REQUIRED)
target_link_libraries(unittest PRIVATE magicqoi Catch2::Catch2 magicqoi_common)
