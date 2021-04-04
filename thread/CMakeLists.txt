
add_library(thread SHARED thread.c mutex.c debug.h)
install(TARGETS thread DESTINATION lib)

message(${CMAKE_BUILD_TYPE})
if(CMAKE_BUILD_TYPE MATCHES Debug)
	target_compile_options(thread PRIVATE "-DUSE_DEBUG")
endif(CMAKE_BUILD_TYPE MATCHES Debug)
