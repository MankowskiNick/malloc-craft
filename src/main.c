#include <stdio.h>
#include <window.h>
#include <render.h>
#include <camera.h>
#include <input.h>

int main() {
    GLFWwindow* window = create_window("malloc-craft", 800, 600);

    if (!window || !load_gl()) {
        return -1;
    }

    shader_program program;
    r_init(&program);

    camera cam = {
        .position = {1.0f, 1.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .front = {1.0f, 0.0f, 0.0f},
        .yaw = 0.0f,
        .pitch = 0.0f
    };

    i_init(window, &cam);

    while (!glfwWindowShouldClose(window)) {
        update_camera();
        render(cam, program);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    r_cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}