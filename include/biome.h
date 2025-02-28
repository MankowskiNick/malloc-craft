#ifndef BIOME_H
#define BIOME_H

typedef struct {
    short id;
    char* name;
    short surface_type, subsurface_type, underground_type, underwater_type;
} biome;

enum {
    PLAINS,
    DESERT,
    // PLAINS,
    // DESERT,
    // OCEAN,
};

biome* get_biome(float x, float z);

extern biome BIOMES[];

#endif