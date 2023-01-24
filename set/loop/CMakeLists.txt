project(loop VERSION 0.0.1)


include_directories(${CMAKE_SOURCE_DIR}/set/thread_pool)
include_directories(${CMAKE_SOURCE_DIR}/set/rtt)


add_executable(loop main.cpp Loop.cpp)
target_link_libraries(loop pthread)

add_library(lloop Loop.cpp)