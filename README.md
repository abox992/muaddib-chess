# Overview
*Muaddib* is a **UCI chess engine** written in C++. The engine analyzes future positions and returns the optimal move. This is done using a minimax alpha-beta pruning search algorithm, along with a static evaluation function.

Muaddib will *eventually* have a GUI for easy play against the engine.

# Requirements/Dependencies
- Cmake version >= 3.21 (for presets)
- C++20

# Compile/Build
**This project is intended to be built on Unix-like systems.**

From the `src` directory:

Debug keeps assertions and skips optimization:
```
cmake --preset debug
cmake --build --preset debug
```

Release is `-O3` and defines `NDEBUG`, which removes `assert()`:
```
cmake --preset release
cmake --build --preset release
```

Binaries are `build/debug/chess` and `build/release/chess`. A configure with no preset is Release.

# Todo
- [X] Move ordering
- [X] PV tracking
- [X] Smarter TT replacement strategy
- [ ] NNUE
- [ ] Multithread
- [X] CLI
- [ ] UCI complete implementation

# About
This is a personal project, I wrote this for fun as I enjoy chess and optimization problems. This has also served as a good way to practice my C++.
