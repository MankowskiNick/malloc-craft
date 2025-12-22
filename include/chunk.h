#ifndef CHUNK_H
#define CHUNK_H

#include <block_models.h>
#include <settings.h>
#include <camera.h>
#include <util.h>


typedef struct {
    int x, z;
} chunk_coord;

uint chunk_hash(chunk_coord c);
int chunk_equals(chunk_coord a, chunk_coord b);
int chunk_coord_equals(void* a, void* b);

void c_init();
void chunk_create(chunk* c, int x, int z);

void set_block_info(chunk* c, int x, int y, int z, short id, short orientation, short rot, short water_level);
void get_block_info(short data, short* id, short* orientation, short* rot, short* water_level);

#endif