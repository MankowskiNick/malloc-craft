#include <world.h>

chunk world[WORLD_SIZE][WORLD_SIZE];

void c_init(chunk* c, int x, int z) {
    c->x = x;
    c->z = z;
    
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (k > 7) {
                    c->blocks[i][k][j] = NULL;  // Air block above level 8
                }
                else if (k == 7) {
                    c->blocks[i][k][j] = &TYPES[1];  // Dirt block below level 8
                }
                else if (k > 4) {
                    c->blocks[i][k][j] = &TYPES[2];  // Grass block below level 8
                }
                else {
                    c->blocks[i][k][j] = &TYPES[3];  // Stone block below level 8
                }
            }
        }
    }
}

void w_init() {
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            c_init(&world[x][z], x, z);
        }
    }
}
void w_cleanup();

chunk* get_chunk(int x, int z) {
    if (x < 0 || x >= WORLD_SIZE || z < 0 || z >= WORLD_SIZE) {
        return NULL;
    }
    chunk *c = &world[x][z];
    return &world[x][z];
}