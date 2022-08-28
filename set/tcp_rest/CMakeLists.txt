project(tcp_rest VERSION 0.0.1)

add_executable(tcp_rest1 main.cpp)
add_executable(tcp_rest2 main2.cpp)
target_link_libraries(
    tcp_rest2 pthread
)
