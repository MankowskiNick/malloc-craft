#ifndef INPUT_H
#define INPUT_H
#include <camera.h>

#include <GLFW/glfw3.h>

void i_init(GLFWwindow* window, camera* player_cam);
void i_cleanup();
void update_camera();

#endif