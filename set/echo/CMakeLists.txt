PROJECT(ECHO VERSION 0.0.1)

add_executable(echo_client echo.client.cpp)

add_executable(echo_server echo.server.cpp)

target_link_libraries(echo_client pthread)