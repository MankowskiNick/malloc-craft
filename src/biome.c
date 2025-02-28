#include <biome.h>
#include <block_models.h>
#include <noise.h>
#include <settings.h>

#include <stdio.h>

short get_biome_id(float x, float z) {
    float noise = n_get(x, z, 
        WORLDGEN_BIOME_FREQUENCY, 
        WORLDGEN_BIOME_AMPLITUDE, 
        1);
    printf("%f  ", noise);
    float biome = noise * (float)BIOME_COUNT; // multiply by number of biomes
    return (uint)biome;
}

biome* get_biome(float x, float z) {
    short id = get_biome_id(x, z);
    return &BIOMES[id];
}

biome BIOMES[] = {
    {
        .id = 0,
        .name = "plains",
        .surface_type = GRASS,
        .subsurface_type = DIRT,
        .underground_type = STONE,
        .underwater_type = DIRT,
    },
    {
        .id = 1,
        .name = "forest",
        .surface_type = GRASS,
        .subsurface_type = DIRT,
        .underground_type = STONE,
        .underwater_type = DIRT,
    },
    {
        .id = 2,
        .name = "desert",
        .surface_type = SAND,
        .subsurface_type = SAND,
        .underground_type = STONE,
        .underwater_type = SAND,
    },
    {
        .id = 3,
        .name = "mountains",
        .surface_type = STONE,
        .subsurface_type = STONE,
        .underground_type = STONE,
        .underwater_type = STONE,
    },
};