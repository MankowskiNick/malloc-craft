cmake_minimum_required(VERSION 3.10)

# Set the project name
project(malloc-craft)

# Default to Debug if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

# Set compiler flags for Debug and Release
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

add_subdirectory(lib/GLFW)
add_subdirectory(lib/cglm)

# Add an executable
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(malloc-craft ${SOURCES})

target_include_directories(malloc-craft PUBLIC include src src/player/core src/player/input src/player/serialization src/world/core src/world/generation src/world/physics src/mesh/core src/mesh/geometry src/mesh/generation src/mesh/world src/mesh/utilities src/render/core src/render/geometry src/render/effects src/render/world src/render/entities src/render/environment src/render/ui)
target_link_libraries(malloc-craft glfw)
target_link_libraries(malloc-craft cglm)