project(sigurg VERSION 0.0.1)

add_executable(sigurg main.cpp)

target_link_libraries(sigurg pthread)
