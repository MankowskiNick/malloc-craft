#include <block.h>
#include <world.h>
#include <cglm/cglm.h>


// TODO: implement actual DDA algorithm
void get_block_coords(camera cam, uint* x, uint* y, uint* z, chunk** chunk_out) {
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;

    while (t < MAX_REACH) {
        t += 0.005f;

        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};

        uint chunk_x, chunk_z;
        chunk* c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);

        if (c->blocks[chunk_x][(int)pos[1]][chunk_z] != NULL) {
            *x = chunk_x;
            *y = (int)pos[1];
            *z = chunk_z;
            *chunk_out = c;
            return;
        }
    }
    *x = -1;
    *y = -1;
    *z = -1;
    *chunk_out = NULL;
}

void break_block(camera cam) {
    uint x, y, z;
    chunk* c = NULL;
    get_block_coords(cam, &x, &y, &z, &c);
    
    if (x == -1 || y == -1 || z == -1 || c == NULL) {
        return;
    }

    c->blocks[x][y][z] = NULL;
}

void place_block(camera cam) {
    uint x, y, z;
    uint side;
    chunk* c = get_chunk_at(cam.position[0], cam.position[2], &x, &z);



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