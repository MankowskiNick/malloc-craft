cmake_minimum_required(VERSION 3.10)

# Set the project name
project(CCraft)

add_subdirectory(lib/GLFW)

# Add an executable
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(CCraft ${SOURCES})

target_include_directories(CCraft PUBLIC include lib/GLFW/include)
target_link_libraries(CCraft glfw)