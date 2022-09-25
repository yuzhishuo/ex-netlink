project(unixsocket VERSION 0.0.1)

add_executable(unixsocket main.cpp)
target_link_libraries(unixsocket pthread)