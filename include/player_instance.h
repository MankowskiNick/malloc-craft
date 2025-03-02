#ifndef PLAYER_INSTANCE_H
#define PLAYER_INSTANCE_H

#include <camera.h>

typedef struct {
    camera cam;
    short selected_block;
} player_instance;

#endif