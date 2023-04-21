cmake_minimum_required(VERSION 3.22.6)
project(net_alert C)

find_package(libuv REQUIRED)
find_package(glib REQUIRED)
find_package(SndFile REQUIRED)

option(PA_BUILD_SHARED_LIBS "Build dynamic library" OFF)
include(AddGitSubmodule.cmake)
add_git_submodule(portaudio)

include_directories(include portaudio/include)

add_executable(${PROJECT_NAME} src/main.c src/wav.c)
target_link_libraries(${PROJECT_NAME} uv_a)
target_link_libraries(${PROJECT_NAME} glib::glib-2.0)
target_link_libraries(${PROJECT_NAME} glib::gio-2.0)
target_link_libraries(${PROJECT_NAME} SndFile::sndfile)
target_link_libraries(${PROJECT_NAME} portaudio_static)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/alert.wav"
    "${CMAKE_BINARY_DIR}/alert.wav"
    COMMENT "Copying alert.wav to build directory"
)
