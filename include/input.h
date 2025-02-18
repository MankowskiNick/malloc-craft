#ifndef INPUT_H
#define INPUT_H
#include <camera.h>

#include <GLFW/glfw3.h>

#define DELTA_X 0.01f
#define DELTA_Y 0.005f
#define DELTA_Z 0.01f
#define SENSITIVITY 0.01f

void i_init(GLFWwindow* window, camera* player_cam);
void i_cleanup();
void update_camera();
#endif