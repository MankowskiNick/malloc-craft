# malloc-craft
A Minecraft-like voxel game written in C using OpenGL. This project focuses on memory management and modern graphics programming techniques to create a block-based 3D world where players can build and explore.

![image](https://github.com/user-attachments/assets/e3188c52-6d53-47e3-8e73-25d6a2fcdba5)



Key features:
- Custom chunk-based world generation
- Basic block placement and destruction
- First-person camera controls
- Texture atlas support for blocks
- OpenGL-based rendering

## Project Structure

- **include/** – Header files for the project (e.g., `block.h`, `camera.h`, `world.h`).
- **src/** – Source code files.
- **lib/** – Third-party libraries (e.g., cglm, GLFW).
- **res/** – Resources such as textures (e.g., `atlas.png`) and shader files.
- **CMakeLists.txt** – Main CMake configuration file.

## Prerequisites

- CMake (version 3.10 or higher recommended)
- A C compiler (e.g., gcc, clang, or MSVC)
- GLFW dependencies (if not using the bundled source in `lib/GLFW`)
- cglm (included in the `lib/cglm` folder)

## Building

1. Create a build directory:
    ```sh
    mkdir build && cd build
    ```

2. Generate the build files with CMake:
    ```sh
    cmake ..
    ```

3. Build the project:
    ```sh
    make
    ```

## Running

After a successful build, the compiled executable `malloc-craft` (or its platform-specific equivalent) will reside in the `build` folder or a subdirectory thereof. Run it from the command line or your IDE.

## Acknowledgments

- [GLFW](https://www.glfw.org/)
- [cglm](https://github.com/recp/cglm)
- [stb](https://github.com/nothings/stb/)
- [glad](https://github.com/Dav1dde/glad)
