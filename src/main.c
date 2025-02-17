#include <stdio.h>
#include <window.h>
#include <render.h>


int main() {
    GLFWwindow* window = create_window("malloc-craft", 800, 600);

    if (!window || !load_gl()) {
        return -1;
    }

    r_init();

    while (!glfwWindowShouldClose(window)) {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    r_cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}