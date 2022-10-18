

add_executable(
    atomic1 main.cpp
)

target_link_libraries(
    atomic1 pthread
)


add_executable(
    atomic2 main1.cpp
)

target_link_libraries(
    atomic2 pthread
)

add_executable(
    atomic3 main2.cpp
)

target_link_libraries(
    atomic3 pthread
)