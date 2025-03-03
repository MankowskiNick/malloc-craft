#include <block.h>
// #include <render.h>
#include <mesh.h>
#include <world.h>
#include <player_instance.h>
#include <cglm/cglm.h>
#include <glad/glad.h>


float get_empty_dist(camera cam) {
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;

    uint chunk_x, chunk_y, chunk_z;
    chunk_y = (uint)position[1];
    chunk* c = get_chunk_at(position[0], position[2], &chunk_x, &chunk_z);

    while (chunk_y >= 0 && chunk_y < CHUNK_HEIGHT 
        && t <= MAX_REACH && 
        c->blocks[chunk_x][chunk_y][chunk_z] == AIR || c->blocks[chunk_x][chunk_y][chunk_z] == WATER) {
        t += 0.005f;

        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};

        c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);
        chunk_y = (uint)pos[1];
    }

    return t;
}

void break_block(player_instance player) {
    camera cam = player.cam;
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

    c->blocks[chunk_x][chunk_y][chunk_z] = AIR;
    
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z);
    queue_chunk_for_sorting(new_mesh);
}

void place_block(player_instance player) {
    camera cam = player.cam;
    float t = get_empty_dist(cam);
    t -= RAY_STEP;

    if (t >= MAX_REACH - RAY_STEP) {
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

    c->blocks[chunk_x][chunk_y][chunk_z] = player.selected_block;

    // update chunk and adjacent chunks
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z);
    queue_chunk_for_sorting(new_mesh);
}

// TODO: refactor to make more safe
block_type* get_block_type(short id) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (TYPES[i].id == id) {
            return &TYPES[i];
        }
    }
    return NULL;
}

void send_cube_vbo(VAO vao, VBO vbo) {
    float faceVertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, faceVertices, 6 * 3 * sizeof(float));
    f_add_attrib(&vbo, 0, 3, 0, 3 * sizeof(float)); // position
    #include <vao.h>
    #include <vbo.h>
    use_vbo(vbo);
}


block_type TYPES[] = {
    {
        .id = 0,
        .name = "air",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {-1, -1},
            {-1, -1},
            {-1, -1},
            {-1, -1},
            {-1, -1},
            {-1, -1},
        }
    },
    {
        .id = 1,
        .name = "grass",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {1, 0},
            {1, 0},
            {1, 0},
            {1, 0},
            {0, 0},
            {2, 0},
        }
    },
    {
        .id = 2,
        .name = "dirt",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {2, 0},
            {2, 0},
            {2, 0},
            {2, 0},
            {2, 0},
            {2, 0},
        }
    },
    {
        .id = 3,
        .name = "stone",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {3, 0},
            {3, 0},
            {3, 0},
            {3, 0},
            {3, 0},
            {3, 0},
        }
    },
    {
        .id = 4,
        .name = "weezer",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            
            {4, 0},
            {4, 0},
            {4, 0},
            {4, 0},
            {5, 0},
            {5, 0},
        }
    },
    {
        .id = 5,
        .name = "oak_log",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {7, 0},
            {7, 0},
            {7, 0},
            {7, 0},
            {8, 0},
            {8, 0},
        }
    },
    {
        .id = 6,
        .name = "oak_leaves",
        .liquid = 0,
        #ifdef TRANSPARENT_LEAVES
            .transparent = 1,
            .face_atlas_coords = {
                {9, 0},
                {9, 0},
                {9, 0},
                {9, 0},
                {9, 0},
                {9, 0},
            }
        #else
            .transparent = 0,
            .face_atlas_coords = {
                {15, 0},
                {15, 0},
                {15, 0},
                {15, 0},
                {15, 0},
                {15, 0},
            }
        #endif
    },
    {
        .id = 7,
        .name = "water",
        .transparent = 1,
        .liquid = 1,
        .face_atlas_coords = {
            {10, 0},
            {10, 0},
            {10, 0},
            {10, 0},
            {10, 0},
            {10, 0},
        }
    },
    {
        .id = 8,
        .name = "glass",
        .liquid = 0,
        .transparent = 1,
        .face_atlas_coords = {
            {11, 0},
            {11, 0},
            {11, 0},
            {11, 0},
            {11, 0},
            {11, 0},
        }
    },
    {
        .id = 9,
        .name = "sand",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {6, 0},
            {6, 0},
            {6, 0},
            {6, 0},
            {6, 0},
            {6, 0},
        }
    },
    {
        .id = 10,
        .name = "cactus",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {12, 0},
            {12, 0},
            {12, 0},
            {12, 0},
            {13, 0},
            {13, 0},
        }
    },
    {
        .id = 11,
        .name = "cactus_top",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {12, 0},
            {12, 0},
            {12, 0},
            {12, 0},
            {14, 0},
            {13, 0},
        }
    },
    {
        .id = 12,
        .name = "oak_planks",
        .liquid = 0,
        .transparent = 0,
        .face_atlas_coords = {
            {16, 0},
            {16, 0},
            {16, 0},
            {16, 0},
            {16, 0},
            {16, 0},
        }
    }
};