#ifndef INPUT_H
#define INPUT_H
#include <player_instance.h>
#include <chunk_mesh.h>

#include <GLFW/glfw3.h>

void i_init(GLFWwindow* window, game_data* data);
void i_cleanup();
void update_camera(int delta_ms);

#endif