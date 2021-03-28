# *** Variables and default recipe ***
thread_exe=cmake-build-debug/test/test-thread
pthread_exe=cmake-build-debug/test/test-pthread
thread_full=/tmp/${USER}-thread-full
pthread_full=/tmp/${USER}-pthread-full
thread_clean=/tmp/${USER}-thread
pthread_clean=/tmp/${USER}-pthread

all: ${thread_exe} ${pthread_exe}

# *** CMake generation ***
cmake=cmake-build-debug/.init

${cmake}:
	mkdir -p cmake-build-debug
	touch cmake-build-debug/.init

cmake-build-debug/Makefile: ${cmake}
	cmake . -B cmake-build-debug

# *** Compilation and execution ***

${thread_exe}: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug test-thread

${pthread_exe}: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug test-pthread

${thread_full}: ${thread_exe}
	./cmake-build-debug/test/test-thread | tee ${thread_full}

${pthread_full}: ${pthread_exe}
	./cmake-build-debug/test/test-pthread | tee ${pthread_full}

test-thread: clean-test ${thread_full}
test-pthread: clean-test ${pthread_full}

# *** Diff ***

${thread_clean}: ${thread_full}
	<${thread_full} grep "TEST::" | sort >${thread_clean}

${pthread_clean}: ${pthread_full}
	<${pthread_full} grep "TEST::" | sort >${pthread_clean}

check-tests: ${thread_clean} ${pthread_clean}
	diff ${pthread_clean} ${thread_clean} --color=always

# *** Cleaning up ***
.PHONY: clean clean-test

clean:
	rm -rf cmake-build-debug

clean-test:
	rm -f ${thread_full} ${pthread_full}
