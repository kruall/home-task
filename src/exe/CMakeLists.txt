add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE core)

add_executable(server_test server_test.cpp)
target_link_libraries(server_test PRIVATE core actors)