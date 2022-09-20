project(inventory)

add_executable(inventory main.cpp)

target_link_libraries(inventory pthread)