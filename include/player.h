#ifndef player_H
#define player_H

#include <camera.h>
#include <cerialize/cerialize.h>

typedef struct {
    camera cam;

    float* position;
    float height;

    int selected_block;
    char** hotbar;
    int hotbar_size;
} player;

player player_init(char* player_file);
void update_player_pos(player* player, float direction[3]);

#endif