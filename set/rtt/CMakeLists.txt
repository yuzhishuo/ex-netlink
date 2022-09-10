project(rtt VERSION 0.0.1)

add_executable(rttserver rtt_server.cpp)

target_link_libraries(rttserver resolv)

add_executable(rttclient rtt_client.cpp)
target_link_libraries(rttclient resolv)