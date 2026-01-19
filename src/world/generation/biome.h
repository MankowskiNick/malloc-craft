#ifndef BIOME_H
#define BIOME_H

#include <util.h>

typedef struct {
    char* type;
    float density;
} foliage;

typedef struct {
    short id;
    char* name;
    char* surface_type;
    char* subsurface_type;
    char* underground_type;
    char* underwater_type;
    foliage* foliage;
    int foliage_count;
} biome;

void read_biomes(char* filename);
biome* get_biome(float x, float z);

extern biome* BIOMES;

#endif
