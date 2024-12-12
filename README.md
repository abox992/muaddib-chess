# Overview
*Muaddib* is a **UCI chess engine** written in C++. The engine analyzes future positions and returns the optimal move. This is done using a minimax alpha-beta pruning search algorithm, along with a static evaluation function.

Muaddib will *eventually* have a GUI for easy play against the engine.

# Requirements/Dependencies
- Cmake version >= 3.5.1
- C++20

# Compile/Build
**This project is intended to be built on Unix-like systems.**

Create and navigate to the build directory:
```
mkdir build
cd build
```
Build with cmake:
```
cmake ../src
make
```
Run the engine:
```
./chess
```

# About
This is a personal project, I wrote this for fun as I enjoy chess and optimization problems. This has also served as a good way to practice my C++.
