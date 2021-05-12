# Projet d'OS, S8 2021

## Development

##### Dependencies

- `make`
- `cmake`
- `glibc`

##### Generic make commands

This project is orchestrated by CMake.

The first time you run the project, you will need to run:
```bash
mkdir cmake-build-debug

# To compile in Debug mode (slower, better logs)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug

# To compile in Release mode (faster, no logs)
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
```

After that, you can use:

```bash
# Debug mode:
make -s -C cmake-build-debug -j <targets>

# Release mode:
make -s -C cmake-build-release -j <targets>
```

The available targets are:

- `all` to compile everything
- `install` to add the tests to `install/bin`
- `test` to execute all tests (with both our implementation and GNU's `pthread`), doesn't compile automatically
- `graphs` to generate a graph of performance for a specific test (to select the test, use the environment variables `test_id=<integer>` and `test_runs=<integer>`, see [graphs.py](graphs.py) for the test IDs)

##### Projet versions

The `master` branch has:

- All the basic functions
- Mutexes
- Deadlock detection

The `signals` branch has:

- Everything in `master`
- An implementation of signals (not merged in `master` because of memory leaks)

##### Code conventions

- [Coding style](https://gitlab.com/braindot/legal/-/blob/master/coding-style/STYLE_C.md)
