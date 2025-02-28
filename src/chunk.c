#include <chunk.h>
#include <noise.h>
#include <biome.h>

uint chunk_hash(chunk_coord c) {
    uint hash = (unsigned int)((c.x * 73856093) ^ (c.z * 19349663)) + CHUNK_CACHE_SIZE;
    return hash % CHUNK_CACHE_SIZE;
}

int chunk_equals(chunk_coord a, chunk_coord b) {
    return a.x == b.x && a.z == b.z;
}

void c_init() {
    n_init(SEED);
}

int get_block_height(chunk* c, float x, float z) {
    float y_ = n_get(x, z, 
        WORLDGEN_BLOCKHEIGHT_FREQUENCY, 
        WORLDGEN_BLOCKHEIGHT_AMPLITUDE, 
        WORLDGEN_BLOCKHEIGHT_OCTAVES) * WORLDGEN_BLOCKHEIGHT_MODIFIER;

    int y = (int)(y_) + (CHUNK_HEIGHT / 2);
    return y;
}

void generate_blocks(chunk* c, int x, int z) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            // get block height
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;
            
            float y = get_block_height(c, x_, z_);

            // get biome
            biome* b = get_biome(x_, z_);
            
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (k > y) {
                    if (k > WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = AIR;  // Air block above water level
                    }
                    else {
                        c->blocks[i][k][j] = WATER;
                    }
                }
                else if (k == y) {
                    
                    if (k < WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = b->underwater_type;  // Dirt if below water level
                    }
                    else {
                        c->blocks[i][k][j] = b->surface_type;  // Grass
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
}

void chunk_create(chunk* c, int x, int z) {
    if (c == NULL) {
        c = malloc(sizeof(chunk));
    }

    c->x = x;
    c->z = z;

    // float biome = n_get(x, z, WORLDGEN_BIOME_FREQUENCY, WORLDGEN_BIOME_AMPLITUDE, WORLDGEN_BIOME_OCTAVES);

    // printf("Biome: %f\n", biome);

    generate_blocks(c, x, z);
}