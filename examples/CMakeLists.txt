add_executable(mgqoi-decode mgqoi-decode.cpp)
target_link_libraries(mgqoi-decode PRIVATE magicqoi magicqoi_common)

add_executable(mgqoi-encode mgqoi-encode.cpp)
target_link_libraries(mgqoi-encode PRIVATE magicqoi magicqoi_common)