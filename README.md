# 7DOT
Would be a real shame if this side-project were abandoned.

- [7DOT](#7dot)
  - [Clone](#clone)
  - [Build](#build)
  - [Run](#run)


## Clone
This repository contains submodules for external dependencies, so when doing a fresh clone you need to clone recursively:

```
git clone --recursive https://github.com/Junkyard-Logic-Studios/7DOT.git
```

Existing repositories can be updated manually:

```
git submodule init
git submodule update
```


## Build
Within the toplevel directory of the repository, run:
```
./build.sh
```

The build script configures CMake, creates or reuses the `build/` directory, builds the project, and then offers to run the game, run tests, or skip running anything.

Common variants:
```
./build.sh --run normal
./build.sh --run test
./build.sh --release --run normal
./build.sh --clean
```

By default the script creates local `.vscode/launch.json` and `.vscode/tasks.json` files when they are missing.


## Run
From the toplevel directory of the repository, run with:
```
./build.sh --run normal
```

If you have built tests in the previous step, these can be executed with:
```
./build.sh --run test
```

The built game executable is still available at `build/bin/7dot`.
