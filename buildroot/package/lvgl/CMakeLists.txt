cmake_minimum_required(VERSION 3.12.4)
project(lvgl_linux C)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.upstream.txt)
# The upstream CMake file creates these targets unconditionally.
set_target_properties(lvgl_examples lvgl_demos PROPERTIES EXCLUDE_FROM_ALL TRUE)

option(BUILD_LVGL_DEMO "Build Linux framebuffer test demos" OFF)
if(BUILD_LVGL_DEMO)
    add_executable(lvgl_demo lvgl_demo.c)
    target_link_libraries(lvgl_demo PRIVATE lvgl_demos lvgl m)
    install(TARGETS lvgl_demo RUNTIME DESTINATION bin)
endif()
