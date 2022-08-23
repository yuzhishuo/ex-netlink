
add_executable(net_time_server server.cpp)
link_libraries(net_time_server pthread)

add_executable(net_time_client client.cpp)