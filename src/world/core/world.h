#ifndef WORLD_H
#define WORLD_H

#include <block_models.h>
#include <util/settings.h>

void w_init();
void w_cleanup();

chunk* get_chunk(int x, int z);
chunk* get_chunk_at(float x, float z, int* chunk_x, int* chunk_z);
const char* get_worlds_dir(void);

#endif
