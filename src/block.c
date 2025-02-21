#include <block.h>


// TODO: implement actual DDA algorithm
void get_block_coords(camera cam, chunk* c, chunk* adj, int* x, int* y, int* z, uint* side) {
    // use dda to get block coords
    float ray[3] = {cam.front[0], cam.front[1], cam.front[2]};
    float pos[3] = {cam.position[0], cam.position[1], cam.position[2]};
    float t = 0.0f;

    while (t < 10.0f) {
        int x_ = (int)pos[0] % (2 * CHUNK_SIZE);
        int y_ = (int)pos[1];
        int z_ = (int)pos[2] % (2 * CHUNK_SIZE);

        if (x_ >= 0 && x_ < CHUNK_SIZE && y_ >= 0 && y_ < CHUNK_HEIGHT && z_ >= 0 && z_ < CHUNK_SIZE) {
            if (c->blocks[x_][y_][z_] != NULL) {
                *x = x_;
                *y = y_;
                *z = z_;
                return;
            }
        } else if (adj != NULL && x_ >= CHUNK_SIZE && x_ < CHUNK_SIZE * 2 && y_ >= 0 && y_ < CHUNK_HEIGHT && z_ >= 0 && z_ < CHUNK_SIZE) {
            if (adj->blocks[x_ - CHUNK_SIZE][y_][z_] != NULL) {
                *x = x_ - CHUNK_SIZE;
                *y = y_;
                *z = z_;
                return;
            }
        }

        pos[0] += ray[0];
        pos[1] += ray[1];
        pos[2] += ray[2];
        t += 0.01f;
    }

    *x = -1;
    *y = -1;
    *z = -1;
}

void break_block(camera cam, chunk* c, chunk* adj) {
    int x, y, z;
    uint side;
    get_block_coords(cam, c, adj, &x, &y, &z, &side);

    if (x == -1 || y == -1 || z == -1) {
        return;
    }
    if (c->blocks[x][y][z] != NULL) {
        c->blocks[x][y][z] = NULL;
    }
}

void place_block(camera cam, chunk* c, chunk* adj, block_type* type) {
    int x, y, z;
    uint side;
    get_block_coords(cam, c, adj, &x, &y, &z, &side);

    if (x == -1 || y == -1 || z == -1) {
        return;
    }
    // if (c->blocks[x][y][z] == NULL) { // temporarily just add above
        c->blocks[x][y + 1][z] = type;
    // }
}


block_type TYPES[] = {
    {
        .id = 0,
        .name = "air",
        .face_atlas_coords = {
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
            {-1.0f, -1.0f},
        }
    },
    {
        .id = 1,
        .name = "grass",
        .face_atlas_coords = {
            {1.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f},
            {1.0f / 32.0f, 0.0f},
            {0.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
        }
    },
    {
        .id = 2,
        .name = "dirt",
        .face_atlas_coords = {
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
            {2.0f / 32.0f, 0.0f},
        }
    },
    {
        .id = 3,
        .name = "stone",
        .face_atlas_coords = {
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
            {3.0f / 32.0f, 0.0f},
        }
    }
};

float CUBE_VERTICES[] = {

    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, FRONT,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f, FRONT,
    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,
 
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,
     -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, BACK,
     -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
     -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BACK,
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,

    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, LEFT,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, LEFT,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, LEFT,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, LEFT,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, LEFT,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, LEFT,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, RIGHT,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, RIGHT,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, RIGHT,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, RIGHT,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, RIGHT,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, RIGHT,


    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, TOP,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
   -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, TOP,
   -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP,
   
   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM,
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f, BOTTOM,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
   -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BOTTOM,
   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM
 };