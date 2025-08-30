#ifndef PLAYER_INSTANCE_H
#define PLAYER_INSTANCE_H

#include <camera.h>
#include <cerialize/cerialize.h>

typedef struct {
    camera cam;
    int selected_block;
    char** hotbar;
    int hotbar_size;
} player_instance;

player_instance player_init(char* player_file);

#endif