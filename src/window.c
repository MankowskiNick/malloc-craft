#include <window.h>
#include <stdio.h>

GLFWwindow* create_window(char* title, int width, int height) {
    if (!glfwInit()) {
        printf("GLFW initialization failed\n");
        return NULL;
    }

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!window) {
        printf("Window or OpenGL context creation failed\n");
        return NULL;
    }
    glfwMakeContextCurrent(window);

    return window;
}

int load_gl() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return 0;
    }
    return 1;
}