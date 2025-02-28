#ifndef BIOME_H
#define BIOME_H

#include <util.h>

typedef struct {
    short id;
    char* name;
    short surface_type, subsurface_type, underground_type, underwater_type;
} biome;

enum {
    PLAINS,
    DESERT,
    MOUNTAINS,
    // PLAINS,
    // DESERT,
    // OCEAN,
};

biome* get_biome(float x, float z);

extern biome BIOMES[];

#endif