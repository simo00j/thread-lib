
set(test-files
    main.c
    thread-utils.c thread-utils.h
    test-utils.h
    )

add_executable(test-thread ${test-files})
target_link_libraries(test-thread thread)

add_executable(test-pthread ${test-files})
target_link_libraries(test-pthread pthread)
target_compile_options(test-pthread PRIVATE "-DUSE_PTHREAD")
