#ifndef INPUT_H
#define INPUT_H
#include <player_instance.h>

#include <GLFW/glfw3.h>

void i_init(GLFWwindow* window, player_instance* player_ptr);
void i_cleanup();
void update_camera();

#endif