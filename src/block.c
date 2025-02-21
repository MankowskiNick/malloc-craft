#include <block.h>
#include <world.h>
#include <cglm/cglm.h>

float get_empty_dist(camera cam) {
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;

    uint chunk_x, chunk_y, chunk_z;
    chunk_y = (uint)position[1];
    chunk* c = get_chunk_at(position[0], position[2], &chunk_x, &chunk_z);

    while (t <= MAX_REACH && c->blocks[chunk_x][chunk_y][chunk_z] == NULL) {
        t += 0.005f;

        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};

        c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);
        chunk_y = (uint)pos[1];
    }

    return t;
}

void break_block(camera cam) {
    float t = get_empty_dist(cam);
    t += RAY_STEP;

    if (t > MAX_REACH) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    uint chunk_x = 0;
    uint chunk_y = 0;
    uint chunk_z = 0;
    chunk_y = (uint)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    c->blocks[chunk_x][chunk_y][chunk_z] = NULL;
}

void place_block(camera cam) {
    
    float t = get_empty_dist(cam);
    t -= RAY_STEP;

    if (t >= MAX_REACH) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    uint chunk_x = 0;
    uint chunk_y = 0;
    uint chunk_z = 0;
    chunk_y = (uint)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    c->blocks[chunk_x][chunk_y][chunk_z] = &TYPES[3];
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