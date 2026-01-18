#include <window.h>
#include <settings.h>
#include <stdio.h>

typedef struct {
    int width;
    int height;
    char* title;
    GLFWwindow* window;
} window;

window screen;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    screen.width = width;
    screen.height = height;
}


GLFWwindow* create_window(char* title, int width, int height) {
    if (!glfwInit()) {
        printf("GLFW initialization failed\n");
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        printf("Window creation failed\n");
        return NULL;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    screen.width = width;
    screen.height = height;
    screen.title = title;
    screen.window = window;

    if (!window) {
        printf("Window or OpenGL context creation failed\n");
        return NULL;
    }
    glfwMakeContextCurrent(window);


    if (VSYNC) {
        glfwSwapInterval(0);
    }

    return window;
}

int load_gl() {

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return 0;
    }

    gladLoadGL();

    glViewport(0, 0, screen.width, screen.height);

    return 1;
}

int get_screen_width() {
    return screen.width;
}

int get_screen_height() {
    return screen.height;
}