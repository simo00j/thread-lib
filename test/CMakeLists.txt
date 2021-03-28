
set(test-files
    thread-utils.c thread-utils.h
    test-utils.h
    )

add_library(thread-test-utils SHARED ${test-files})
target_link_libraries(thread-test-utils thread)

add_library(pthread-test-utils SHARED ${test-files})
target_link_libraries(pthread-test-utils thread)
target_compile_options(pthread-test-utils PRIVATE "-DUSE_PTHREAD")
