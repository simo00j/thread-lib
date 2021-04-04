
set(files
    01-main.c
#    02-switch.c
#    11-join.c
#    12-join-main.c
#    21-create-many.c
#    22-create-many-recursive.c
#    23-create-many-once.c
#    31-switch-many.c
#    32-switch-many-join.c
#    33-switch-many-cascade.c
#    51-fibonacci.c
#    61-mutex.c
#    62-mutex.c
    )

foreach (file ${files})

	string(REGEX REPLACE "\\.[^.]*$" "" file_no_ext ${file})
	get_filename_component(file_cleaned ${file} NAME_WE)

	set(thread_impl ${file_cleaned})
	set(pthread_impl ${file_cleaned}-pthread)

	# Tests with our thread
	add_executable(${thread_impl} ${file})
	target_link_libraries(${thread_impl} thread)
	target_include_directories(${thread_impl} PUBLIC ..)
	add_test(${thread_impl} ${thread_impl} 4 4)
	install(TARGETS ${thread_impl} DESTINATION bin)

	# Tests with pthread
	add_executable(${pthread_impl} ${file})
	target_link_libraries(${pthread_impl} pthread)
	target_compile_options(${pthread_impl} PRIVATE "-DUSE_PTHREAD")
	target_include_directories(${pthread_impl} PUBLIC ..)
	add_test(${pthread_impl} ${pthread_impl} 4 4)

endforeach ()
