#ifndef WORLD_H
#define WORLD_H

#include <block_models.h>
#include <settings.h>

void w_init();
void w_cleanup();

chunk* get_chunk(int x, int z);
chunk* get_chunk_at(float x, float z, uint* chunk_x, uint* chunk_z);

#endif