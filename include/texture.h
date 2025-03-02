#ifndef TEXTURE_H
#define TEXTURE_H

#include <util.h>

#define TEX_WIDTH 16
#define TEX_HEIGHT 16

typedef struct {
    uint id;
    uint width, height;
    uint num_channels;
    uint tex_index;
} texture;

texture t_init(char* path, uint tex_index);
void t_cleanup(texture* tex);

#endif