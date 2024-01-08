# Dark-Star project

Dark-Star is a n-body gravity simulator I am developing to learn new things.

It simulates a 3D environment and renders it using OpenGL.    
Gravity calculation uses Newton's gravity equation.

The project is in very early development process.

# Gravity solvers

Currently, the following gravity solvers are implemented:

- Single-threaded Bruteforce algorithm with O(n^2) complexity.
- Multi-threaded Bruteforce algorithm with O(n^2 / p) complexity.
- Single-threaded Barnes Hut algorithm.
- Multi-threaded Barnes Hut algorithm.

Note: **n** is the number of bodies and **p** is the number of processors.

# Dependencies

To compile, you need a C++23 capable compiler and the following libraries:

- SDL
- OpenGL
- My-lib (https://github.com/ehmcruz/my-lib). The Makefile is configured to search **my-lib** in the same parent folder as this repository. If you put somewhere else, just modify the Makefile.
- My-game-library (https://github.com/ehmcruz/my-game-lib)
- Thread Pool library (https://github.com/bshoshany/thread-pool)
