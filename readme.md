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
- My-lib (https://github.com/ehmcruz/my-lib).
- My-game-library (https://github.com/ehmcruz/my-game-lib)
- Thread Pool library (https://github.com/bshoshany/thread-pool)

The Makefile is configured to search **my-lib** and **my-game-lib** in the same parent folder as this repository. If you put somewhere else, just modify the Makefile.

Edit in the Makefile the path to **thread-pool**.

# Compiling

I only tested in Linux, but should compile on any system that has the required dependencies.

**make MYGLIB_TARGET_LINUX=1**

# Downloading assets

I don't like to store assets in GitHub.
So download them from my drive:

**https://drive.google.com/drive/folders/1FlMhFVxTusTblZXbPpf1nIecc8KejfXw?usp=sharing**

Just put the assets folder in project's root directory.

# Running

**./examples/test.exe**

Remember that Dark-Star is more of a library than a standalone software.
You can use it to test any gravitational system.
Just set up the bodies as needed.