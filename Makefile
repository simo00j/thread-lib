# *** Variables and default recipe ***
thread_number ?= 4
yield_number ?= 4

compile: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug

# *** CMake generation ***
cmake=cmake-build-debug/.init

${cmake}:
	mkdir -p cmake-build-debug
	touch cmake-build-debug/.init

cmake-build-debug/Makefile: ${cmake}
	cmake . -B cmake-build-debug

# *** Test battery ***
.PRECIOUS: cmake-build-debug/test/battery/% /tmp/${USER}-% /tmp/${USER}-%-diff

cmake-build-debug/test/battery/%: cmake-build-debug/Makefile
	$(MAKE) -s -C cmake-build-debug $(shell basename $@)

/tmp/${USER}-%: cmake-build-debug/test/battery/%
	(./cmake-build-debug/test/battery/$(shell basename $<) ${thread_number} ${yield_number} || echo "TEST::CRASH" >>$@) | tee $@

/tmp/${USER}-%-diff: /tmp/${USER}-%-pthread /tmp/${USER}-%-thread
	diff $^ --color=always | grep "TEST::" | tee $@

.PHONY: valgrind-%
valgrind-%: cmake-build-debug/test/battery/%
	valgrind --tool=memcheck --gen-suppressions=all --leak-check=full --leak-resolution=med --track-origins=yes ./cmake-build-debug/test/battery/$(shell basename $<) ${thread_number} ${yield_number}

.PHONY: run-%
run-%: /tmp/${USER}-%
	@echo "Execution done." >/dev/null

tests := $(patsubst %.c,%,$(wildcard test/battery/*.c))
.PHONY: check-all
check-all: $(patsubst test/battery/%,check-%,$(tests))
	@echo "Checking all scripts done." >/dev/null

.PHONY: check-%
check-%: /tmp/${USER}-%-diff
	@echo "Checking $@ done." >/dev/null

# *** Installation (required by the Forge) ***

install/bin/.init:
	mkdir -p install/bin
	touch install/bin/.init

install/bin/%: cmake-build-debug/test/battery/% install/bin/.init
	cp $< $@

.PHONY: install
install: $(patsubst test/battery/%,install/bin/%-thread,$(tests))
	@echo "Installation done."

# *** Cleaning up ***
.PHONY: clean clean-test

clean: clean-test
	rm -rf cmake-build-debug install

clean-test:
	rm -f /tmp/${USER}-*-thread /tmp/${USER}-*-pthread /tmp/${USER}-*-diff
