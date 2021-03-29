
file(GLOB files "*.c")

foreach (file ${files})

	string(REGEX REPLACE "\\.[^.]*$" "" file_no_ext ${file})
	get_filename_component(file_cleaned ${file} NAME_WE)

	set(thread_impl ${file_cleaned}-thread)
	set(pthread_impl ${file_cleaned}-pthread)

	# Tests with our thread
	add_executable(${thread_impl} ${file})
	target_link_libraries(${thread_impl} thread)
	target_link_libraries(${thread_impl} thread-test-utils)
	target_include_directories(${thread_impl} PUBLIC ..)

	# Tests with pthread
	add_executable(${pthread_impl} ${file})
	target_link_libraries(${pthread_impl} pthread)
	target_link_libraries(${pthread_impl} pthread-test-utils)
	target_compile_options(${pthread_impl} PRIVATE "-DUSE_PTHREAD")
	target_include_directories(${pthread_impl} PUBLIC ..)

endforeach ()
