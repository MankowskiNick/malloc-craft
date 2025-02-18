#ifndef TEXTURE_H
#define TEXTURE_H

#include <util.h>

#define TEX_WIDTH 16
#define TEX_HEIGHT 16

typedef struct {
    uint id;
    uint width, height;
    uint num_channels;
} atlas;

void t_init();
void t_cleanup();

extern atlas texture_atlas;



#endif