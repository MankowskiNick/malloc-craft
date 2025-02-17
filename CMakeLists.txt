cmake_minimum_required(VERSION 3.10)

# Set the project name
project(malloc-craft)

add_subdirectory(lib/GLFW)

# Add an executable
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(malloc-craft ${SOURCES})

target_include_directories(malloc-craft PUBLIC include lib/GLFW/include)
target_link_libraries(malloc-craft glfw)