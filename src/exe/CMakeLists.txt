add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world core)

add_executable(server_test server_test.cpp)
target_link_libraries(server_test core actors)

add_executable(client_server_test client_server_test.cpp)
target_link_libraries(client_server_test core actors)