cmake_minimum_required(VERSION 3.16)
project(os_s8 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
enable_testing()

add_compile_options(-Wall -Werror -pedantic)

# Necessary because of the use of 'assert' in tests, which are not present in 'release'
add_compile_options(-Wno-error=unused-but-set-variable)

include_directories(include)
add_subdirectory(thread)
add_subdirectory(test)
add_subdirectory(test/battery)
