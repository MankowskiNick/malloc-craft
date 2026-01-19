#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

GLFWwindow* create_window(char* title, int width, int height);
int load_gl();
int get_screen_width();
int get_screen_height();
void toggle_fullscreen(GLFWwindow* window);

#endif
