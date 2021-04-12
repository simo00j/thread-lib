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
cmake -B cmake-build-debug
```

After that, you can use:
```bash
make -s -C cmake-build-debug -j <targets>
```
The available targets are:
- `all` to compile everything
- `install` to add the tests to `install/bin`
- `test` to execute all tests (with both our implementation and GNU's `pthread`)
- `graphs` to generate a graph of performance for a specific test (to select the test, use the environment variables `test_id=<integer>` and `test_runs=<integer>`)

##### Code conventions

- [Coding style](https://gitlab.com/braindot/legal/-/blob/master/coding-style/STYLE_C.md)
