#include <window.h>
#include <stdio.h>

typedef struct {
    int width;
    int height;
    char* title;
    GLFWwindow* window;
} window;

window screen;

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

    screen.width = width;
    screen.height = height;
    screen.title = title;
    screen.window = window;

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

    gladLoadGL();

    glViewport(0, 0, screen.width, screen.height);
    
    return 1;
}