#include <chunk.h>
#include <noise.h>
#include <biome.h>
#include <tree.h>
#include <stdlib.h>

uint chunk_hash(chunk_coord c) {
    uint hash = (unsigned int)((c.x * 73856093) ^ (c.z * 19349663)) + CHUNK_CACHE_SIZE;
    return hash % CHUNK_CACHE_SIZE;
}

int chunk_equals(chunk_coord a, chunk_coord b) {
    return a.x == b.x && a.z == b.z;
}

void c_init() {
    n_init(SEED);
    tree_init();
}

int get_block_height(chunk* c, float x, float z, biome* b) {
    float noise = (n_get(x, z, 
        WORLDGEN_BIOME_FREQUENCY,
        WORLDGEN_BIOME_AMPLITUDE,
        WORLDGEN_BIOME_OCTAVES) - 0.5f) * 2.0f;

    float y_ = WORLDGEN_BASE_TERRAIN_HEIGHT + noise * WORLDGEN_BLOCKHEIGHT_MODIFIER;

    // int y = (int)(y_) + WORLDGEN_BASE_TERRAIN_HEIGHT;
    return (int)y_;
}

void generate_blocks(chunk* c, int x, int z) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;

            biome* b = get_biome(x_, z_);
            float y = get_block_height(c, x_, z_, b);
            
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (k > y) {
                    if (k > WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = AIR;
                    }
                    else {
                        c->blocks[i][k][j] = WATER;
                    }
                }
                else if (k == y) {
                    
                    if (k < WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = b->underwater_type;
                    }
                    else {
                        c->blocks[i][k][j] = b->surface_type;
                    }
                }
                else if (k > y - 3) {
                    c->blocks[i][k][j] = b->subsurface_type;
                }
                else {
                    c->blocks[i][k][j] = b->underground_type;
                }
            }
        }
    }

    // generate trees -- separate pass so it doesn't cut out terrain
    for (int i = 2; i < CHUNK_SIZE - 2; i++) {
        for (int j = 2; j < CHUNK_SIZE - 2; j++) {
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;

            biome* b = get_biome(x_, z_);
            int y = get_block_height(c, x_, z_, b);

            if (rand() / (float)RAND_MAX < b->tree_density && y > WORLDGEN_WATER_LEVEL) {
                generate_tree(i, y, j, b->tree_type, c);
            }
        }
    }
}

void chunk_create(chunk* c, int x, int z) {
    if (c == NULL) {
        c = malloc(sizeof(chunk));
    }

    c->x = x;
    c->z = z;

    generate_blocks(c, x, z);
}