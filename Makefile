
all: check-tests

# *** CMake generation ***

cmake-build-debug:
	mkdir -p cmake-build-debug

cmake-build-debug/Makefile: cmake-build-debug
	cmake . -B cmake-build-debug

# *** Compilation and execution ***
.PHONY: compile-thread compile-pthread test-thread test-pthread check-tests

thread_full=/tmp/${USER}-thread-full
pthread_full=/tmp/${USER}-pthread-full
thread_clean=/tmp/${USER}-thread
pthread_clean=/tmp/${USER}-pthread

compile-thread: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug test-thread

compile-pthread: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug test-pthread

test-thread: compile-thread
	./cmake-build-debug/test/test-thread | tee ${thread_full}

test-pthread: compile-pthread
	./cmake-build-debug/test/test-pthread | tee ${pthread_full}

# *** Diff ***

${thread_clean}: test-thread
	<${thread_full} grep "TEST::" | sort >${thread_clean}

${pthread_clean}: test-pthread
	<${pthread_full} grep "TEST::" | sort >${pthread_clean}

check-tests: ${thread_clean} ${pthread_clean}
	@echo
	diff ${pthread_clean} ${thread_clean} --color=always && echo "Success: no differences!"

# *** Cleaning up ***

clean:
	rm -rf cmake-build-debug
