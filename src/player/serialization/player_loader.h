#ifndef PLAYER_LOADER_H
#define PLAYER_LOADER_H

#include <player/core/player.h>
#include <player/core/camera.h>

// Parse camera from JSON object
camera parse_camera(json_object cam_obj);

// Parse hotbar from JSON object
char** parse_hotbar(json_object hotbar_obj);

// Parse player height from JSON object
float parse_height(json_object height_obj);

// Parse player radius from JSON object
float parse_radius(json_object radius_obj);

// Parse player position from JSON object
float* parse_position(json_object position_object);

#endif
