#include <chunk.h>

void c_init(chunk* c, int x, int z) {
    if (c == NULL) {
        c = malloc(sizeof(chunk));
    }

    // c->id = id;
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

    //make a circular hole in the middle of the chunk at ground level
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                if (sqrt(pow(i - CHUNK_SIZE / 2, 2) + pow(j - CHUNK_SIZE / 2, 2) + pow(k - 8, 2)) < 4) {
                    c->blocks[i][k][j] = NULL;
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
    visible = adjacent == NULL;

    return visible;
}
