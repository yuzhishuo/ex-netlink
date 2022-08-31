project(api VERSION 0.0.2)

add_executable(api main.cpp)
target_link_libraries(api pthread)

add_executable(api1 main1.cpp)
target_link_libraries(api1 pthread)