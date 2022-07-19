add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world core)

add_executable(server_test server_test.cpp)
target_link_libraries(server_test core actors logic)

add_executable(client_server_test client_server_test.cpp)
target_link_libraries(client_server_test core actors logic)

add_executable(client_server_slow client_server_slow.cpp)
target_link_libraries(client_server_slow core actors logic)

add_executable(client_server_fat client_server_fat.cpp)
target_link_libraries(client_server_fat core actors logic)

add_executable(client_server client_server.cpp)
target_link_libraries(client_server core actors logic)