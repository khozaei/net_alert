cmake_minimum_required(VERSION 3.22.6)
project(net_alert C)

find_package(libuv REQUIRED)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} uv_a)
