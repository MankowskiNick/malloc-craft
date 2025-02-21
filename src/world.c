#include <world.h>
#include <chunk.h>

chunk world[WORLD_SIZE][WORLD_SIZE];

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