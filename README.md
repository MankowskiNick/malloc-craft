# malloc-craft
A Minecraft-like voxel game written in C using OpenGL. This project focuses on memory management and modern graphics programming techniques to create a block-based 3D world where players can build and explore.

<img width="1195" alt="grafik" src="https://github.com/user-attachments/assets/e306427b-4685-433a-bbf4-f632135aa5b2" />
<img width="1195" alt="grafik" src="https://github.com/user-attachments/assets/126101ba-31f5-4e5f-aa1f-111a24711e28" />


## Key Features
- Procedural terrain generation with multiple biomes
- Dynamic chunk loading and management
- Block placement and destruction with raycast selection
- Procedural tree generation
- Texture atlas support for various block types
- Performance optimizations with mesh batching

## Settings
Many parts of `malloc-craft` are configurable.  Please see `include/settings.h` for more information.

## Project Structure

- **include/** – Header files defining the game's architecture
- **src/** – Source directory
- **lib/** – Third-party libraries
  - cglm – Math library optimized for graphics programming
  - GLFW – Platform-independent window and input handling
  - glad – OpenGL loading library
  - stb_image – Image loading utilities
- **res/** – Game resources
  - **shaders/** – GLSL shader programs
  - **atlas.png / atlas.xcf** - Texture atlas image and GIMP file
  - **skybox.png / skybox.xcf** - Skybox image and GIMP file

## Prerequisites

- CMake (version 3.10 or higher)
- C compiler with C11 support (gcc, clang, MSVC)
- OpenGL 3.3+ compatible graphics hardware and drivers

## Building

1. Clone the repository with submodules:
    ```sh
    git clone  https://github.com/MankowskiNick/malloc-craft.git
    cd malloc-craft
    ```

2. Create a build directory:
    ```sh
    mkdir build && cd build
    ```

3. Generate the build files with CMake:
    ```sh
    cmake ..
    ```

4. Build the project:
    ```sh
    make
    ```

## Acknowledgments

- [GLFW](https://www.glfw.org/) – Window and input management
- [cglm](https://github.com/recp/cglm) – Graphics math library
- [stb](https://github.com/nothings/stb/) – Image loading
- [glad](https://github.com/Dav1dde/glad) – OpenGL loading
