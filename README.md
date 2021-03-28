# Projet d'OS, S8 2021

## Development

##### Dependencies

- `make`
- `cmake`
- `glibc`

##### Generic make commands

This project is orchestrated by a toplevel `Makefile`. It is crafted to automatically compile everything when necessary.

- `make compile` (default recipe): Force a full compilation
- `make install` Install the binaries in the `install/bin` directory
- `make check-all` Check all implementations (by comparing ours with GNU's `pthread`)
- `make clean` Remove all generated files
- `make clean-test` Mark the test results out-of-date, so the next test command will re-execute them

##### Tests

To select a test, use the following patterns:

- `<name>` is the name of a test (without file extension),
- `<impl>` is the name of the implementation you want to use (`thread` or `pthread`).

These commands are available to execute specific tests:

- `make run-<name>-<impl>` Execute the test
- `make valgrind-<name>-<impl>` Execute the test with valgrind memcheck
- `make check-<name>` Compare the two implementations

For example, to execute the `01-main` test with a `thread` implementation, using `valgrind`, you can execute:

```bash
make valgrind-01-main-thread
```

##### Build/test parameters

All commands (except for the `clean` commands) are thread-safe; you can use `-j` to speedup compilation.

To select the number of threads and yields used in tests, use the environment variables `thread_number` and `yield_number`. For example, to execute `02-switch`'s `thread` implementation with 6 threads and 2 calls to yield, use:

```bash
thread_number=6 yield_number=2 make run-02-switch-thread
```

##### Code conventions

- [Coding style](https://gitlab.com/braindot/legal/-/blob/master/coding-style/STYLE_C.md)
