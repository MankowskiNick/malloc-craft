#ifndef BIOME_H
#define BIOME_H

#include <util.h>

typedef struct {
    short id;
    char* name;
    short surface_type, subsurface_type, underground_type, underwater_type;
    short tree_type;
    float tree_density;
} biome;

enum {
    PLAINS,
    DESERT,
    FOREST,
    MOUNTAINS,
};

biome* get_biome(float x, float z);

extern biome BIOMES[];

#endif