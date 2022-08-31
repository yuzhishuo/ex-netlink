project(api VERSION 0.0.1)

add_executable(api main.cpp)
target_link_libraries(api pthread)