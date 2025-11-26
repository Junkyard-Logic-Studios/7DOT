# 7DOT
Side-project which will inevitably be abandoned sooner or later.

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
Within the toplevel directory of the repository, create a build directory:
```
mkdir build && cd build
```

Then configure the cmake project:
```
cmake .. -DBUILD_TESTS=<ON|OFF>
```

Finally build with:
```
cmake --build . -j 8
```


## Run
From the toplevel directory of the repository, run with:
```
build/bin/7dot
```

If you have built tests in the previous step, these can be executed with:
```
build/bin/7dot_test
```
