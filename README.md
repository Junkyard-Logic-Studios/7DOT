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

## Character editor

Open `Editors` from the main menu, then choose the active `Character Editor`
card (the `Level Editor` card is a disabled placeholder). Built-in archers are
shown as locked cards; characters created in the editor can be edited or
deleted. The editor has separate Bow, Body, and Head canvases sharing aligned
animation ranges for movement, dodge/dash, ledge and wall states, aiming, head
looks, and bow draw/empty states. It supports mouse painting, right-click
erasing, an HSV color wheel with value and alpha sliders, editable palette
swatches, part/animation/frame navigation, and a composited animated preview.

Saved characters are written separately from the original asset submodule to
`custom_assets/Characters/` as `characters.xml`,
`customCharacterAtlas.bmp`, and `customCharacterAtlas.xml`. They are appended
to the in-game character roster the next time character selection is opened.
