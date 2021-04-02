
add_library(thread SHARED thread.c mutex.c)
install(TARGETS thread DESTINATION lib)
