#ifndef CHUNK_H
#define CHUNK_H

#include <block_models.h>
#include <settings.h>
#include <camera.h>


void chunk_create(chunk* c, int x, int z);

int get_side_visible(camera cam, int x, int y, int z, uint side, chunk* c, chunk* adj);

#endif