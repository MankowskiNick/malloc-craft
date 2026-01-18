#ifndef player_H
#define player_H

#include <camera.h>
#include <cerialize/cerialize.h>

typedef struct {
    camera cam;

    float* position;
    float* velocity;
    
    float height;
    float radius;

    int selected_block;
    char** hotbar;
    int hotbar_size;
} player;

player player_init(char* player_file);
void apply_physics(player* player, float delta_ms);

#endif