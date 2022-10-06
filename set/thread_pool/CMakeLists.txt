add_executable(
    thread_pool1 main.cpp
)

target_link_libraries(
    thread_pool1 pthread
)