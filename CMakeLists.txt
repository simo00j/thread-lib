cmake_minimum_required(VERSION 3.16)
project(os_s8 C)

set(CMAKE_C_STANDARD 11)

add_compile_options("-Wall")

include_directories(include)
add_subdirectory(thread)
add_subdirectory(test)
