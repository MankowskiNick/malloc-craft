#include <chunk.h>
#include <noise.h>

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

void chunk_create(chunk* c, int x, int z) {
    if (c == NULL) {
        c = malloc(sizeof(chunk));
    }

    c->x = x;
    c->z = z;

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            // get perlin noise at this point
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;
            float y_ = n_get(x_, z_, 
                    WORLDGEN_BLOCKHEIGHT_FREQUENCY, 
                    WORLDGEN_BLOCKHEIGHT_AMPLITUDE, 
                    WORLDGEN_BLOCKHEIGHT_OCTAVES)
                * WORLDGEN_BLOCKHEIGHT_MODIFIER;

            int y = (int)(y_) + (CHUNK_HEIGHT / 2);

            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (k > y) {
                    if (k > WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = NULL;  // Air block above water level
                    }
                    else {
                        c->blocks[i][k][j] = &TYPES[7];  // Water otherwise
                    }
                }
                else if (k == y) {
                    if (k < WORLDGEN_WATER_LEVEL) {
                        c->blocks[i][k][j] = &TYPES[2];  // Dirt if below water level
                    }
                    else {
                        c->blocks[i][k][j] = &TYPES[1];  // Grass
                    }
                }
                else if (k > y - 3) {
                    c->blocks[i][k][j] = &TYPES[2];  // Dirt 
                }
                else {
                    c->blocks[i][k][j] = &TYPES[3];  // Stone block below level 8
                }
            }
        }
    }
}
