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
                        c->blocks[i][k][j] = NULL;  // Air block above level 8
                    }
                    else {
                        c->blocks[i][k][j] = &TYPES[7];  // Water block above level 8
                    }
                }
                else if (k == y) {
                    c->blocks[i][k][j] = &TYPES[1];  // Dirt block below level 8
                }
                else if (k > y - 3) {
                    c->blocks[i][k][j] = &TYPES[2];  // Grass block below level 8
                }
                else {
                    c->blocks[i][k][j] = &TYPES[3];  // Stone block below level 8
                }
            }
        }
    }
}

void get_adjacent(int x, int y, int z, uint side, chunk* c, chunk* adj, block_type** block) {
    switch(side) {
        case (int)TOP:
            if (y + 1 < CHUNK_HEIGHT) {
                *block = c->blocks[x][y + 1][z];
            }
            else {
                *block = NULL;
            }
            break;
        case (int)BOTTOM:
            if (y - 1 >= 0) {
                *block = c->blocks[x][y - 1][z];
            }
            else {
                *block = NULL;
            }
            break;
        case (int)FRONT:
            if (x + 1 < CHUNK_SIZE) {
                *block = c->blocks[x + 1][y][z];
            }
            else if (adj != NULL) {
                *block = adj->blocks[0][y][z];
            }
            else {
                *block = NULL;
            }
            break;
        case (int)BACK:
            if (x - 1 >= 0) {
                *block = c->blocks[x - 1][y][z];
            }
            else if (adj != NULL) {
                *block = adj->blocks[CHUNK_SIZE - 1][y][z];
            }
            else {
                *block = NULL;
            }
            break;
        case (int)LEFT:
            if (z - 1 >= 0) {
                *block = c->blocks[x][y][z - 1];
            }
            else if (adj != NULL) {
                *block = adj->blocks[x][y][CHUNK_SIZE - 1];
            }
            else {
                *block = NULL;
            }
            break;
        case (int)RIGHT:
            if (z + 1 < CHUNK_SIZE) {
                *block = c->blocks[x][y][z + 1];
            }
            else if (adj != NULL) {
                *block = adj->blocks[x][y][0];
            }
            else {
                *block = NULL;
            }
            break;
        default:
            *block = NULL;
            break;
    }
}

int get_side_visible(
    int x, int y, int z,
    uint side, 
    chunk* c,
    chunk* adj
) {
    uint visible = 0;

    // calculate adjacent block
    block_type* adjacent = NULL;
    get_adjacent(x, y, z, side, c, adj, &adjacent);

    // calculate visibility
    visible = adjacent == NULL;// || adjacent->transparent == c->blocks[x][y][z]->transparent;

    // dont render sides that we can't see
    switch(side) {
        case (int)TOP:
            visible = visible && y < CHUNK_HEIGHT;
            break;
        case (int)BOTTOM:
            visible = visible && y > 0;
            break;
        default:
            break;
    }

    return visible;
}
