cmake_minimum_required(VERSION 3.3)
project(os_s8 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
enable_testing()

add_compile_options(-Wall -Werror -pedantic)

# Necessary because of the use of 'assert' in tests, which are not present in 'release'
add_compile_options(-Wno-error=unused-but-set-variable)
# Necessary because the tests use pointer arithmetics when compiled with pthread
add_compile_options(-Wno-error=pointer-arith)
# Necessary because the tests (eg. fibonacci) use variables that are initialized in loops, which cannot be checked for safety by the compiler
add_compile_options(-Wno-error=maybe-uninitialized)

include_directories(include)
add_subdirectory(thread)
add_subdirectory(test)
add_subdirectory(test/battery)

#region Graphs

add_custom_command(OUTPUT venv/bin/activate
                   COMMAND python3 -m venv venv
                   COMMAND ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/pip install -r ${PROJECT_SOURCE_DIR}/graphs-requirements.txt)

if (NOT DEFINED ENV{test_id})
	set(ENV{test_id} 0)
endif ()

if (NOT DEFINED ENV{test_runs})
	set(ENV{test_runs} 100)
endif ()

add_custom_target(graphs
                  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/python ${PROJECT_SOURCE_DIR}/graphs.py $ENV{test_id} $ENV{test_runs} ${PROJECT_BINARY_DIR}/test/battery
                  DEPENDS venv/bin/activate)

#endregion
