#include <render.h>
#include <block.h>
#include <chunk.h>
#include <world.h>
#include <vbo.h>
#include <vao.h>
#include <texture.h>
#include <assert.h>
#include <hashmap.h>
#include <sort.h>
#include <mesh.h>
#include <queue.h>

#include <settings.h>

#define CAMERA_POS_TO_CHUNK_POS(x) x >= 0 ? (int)(x / CHUNK_SIZE) : (int)(x / CHUNK_SIZE) - 1

VAO vao;
VBO vbo;
shader_program program;

camera* r_cam_ref;

typedef struct {
    float x, z;
    int chunk_x, chunk_z;
} camera_cache;

camera_cache cam_cache;

float chunk_distance_to_camera(const void* item) {
    chunk_mesh* packet = *(chunk_mesh**)item;
    // camera coords to chunk coords
    float x = (r_cam_ref->position[0] / (float)CHUNK_SIZE);
    float z = (r_cam_ref->position[2] / (float)CHUNK_SIZE);

    return -1.0f * sqrt(
        pow((float)(packet->x + 0.5f) - x, 2) +
        pow((float)(packet->z + 0.5f) - z, 2)
    );
}

void r_init(shader_program* program, camera* camera) {
    r_cam_ref = camera;

    cam_cache.x = camera->position[0];
    cam_cache.z = camera->position[2];
    cam_cache.chunk_x = CAMERA_POS_TO_CHUNK_POS(camera->position[0]);
    cam_cache.chunk_z = CAMERA_POS_TO_CHUNK_POS(camera->position[2]);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    #ifdef WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    #endif

    // buffers
    vao = create_vao();
    bind_vao(vao);
    vbo = create_vbo(GL_STATIC_DRAW);

    // shaders and program
    shader frag_shader = create_shader("res/shaders/shader.frag", GL_FRAGMENT_SHADER);
    shader vert_shader = create_shader("res/shaders/shader.vert", GL_VERTEX_SHADER);

    *program = create_program(vert_shader, frag_shader);
    use_program(*program);

    delete_shader(frag_shader);
    delete_shader(vert_shader);

    // texture
    t_init();

    // world
    w_init();

    m_init(camera);

    glViewport(0, 0, WIDTH, HEIGHT);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = cam_cache.chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = cam_cache.chunk_z - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }
}

void r_cleanup() {
    delete_vao(vao);
    delete_vbo(vbo);
    delete_program(program);
    t_cleanup();
    m_cleanup();
}

void render_sides(float* side_data, int num_sides) {
    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, side_data, num_sides * SIDE_OFFSET * sizeof(float));
    add_attrib(&vbo, 0, 3, 0, VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 1, 2, 3 * sizeof(float), VBO_WIDTH * sizeof(float));
    add_attrib(&vbo, 2, 2, 5 * sizeof(float), VBO_WIDTH * sizeof(float));
    use_vbo(vbo);

    glDrawArrays(GL_TRIANGLES, 0, num_sides * VERTS_PER_SIDE);
}

void render(camera cam, shader_program program) {
    mat4 view, proj;
    get_view_matrix(cam, &view);
    uint view_loc = glGetUniformLocation(program.id, "view");
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view);
    
    get_projection_matrix(&proj, 45.0f, 800.0f / 600.0f, 0.1f, RENDER_DISTANCE);
    uint proj_loc = glGetUniformLocation(program.id, "proj");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas.id);
    uint atlas_loc = glGetUniformLocation(program.id, "atlas");
    glUniform1i(atlas_loc, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(cam.position[0]);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(cam.position[2]);


    int movedBlocks = ((int)cam.position[0] == (int)cam_cache.x && (int)cam.position[2] == (int)cam_cache.z) ? 0 : 1;
    if (movedBlocks) {
        cam_cache.x = cam.position[0];
        cam_cache.z = cam.position[2];
    }

    int movedChunks = (player_chunk_x == cam_cache.chunk_x && player_chunk_z == cam_cache.chunk_z) ? 0 : 1;

    // int num_packets = 4 * CHUNK_RENDER_DISTANCE * CHUNK_RENDER_DISTANCE;
    // chunk_mesh* packet[num_packets];
    chunk_mesh** packet = NULL;
    int num_packets = 0;
    // int chunks_loaded = 1;

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = player_chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = player_chunk_z - CHUNK_RENDER_DISTANCE + j;

            if (sqrt(pow(x - player_chunk_x, 2) + pow(z - player_chunk_z, 2)) > CHUNK_RENDER_DISTANCE) {
                continue;
            }

            chunk_mesh* mesh = get_chunk_mesh(x, z);

            // packet[i * 2 * CHUNK_RENDER_DISTANCE + j] = mesh;

            if (mesh == NULL) {
                continue;
            }

            packet = realloc(packet, (num_packets + 1) * sizeof(chunk_mesh*));
            packet[num_packets] = mesh;
            num_packets++;

            if (x >= player_chunk_x - 1 
                && x <= player_chunk_x + 1 
                && z >= player_chunk_z - 1
                && z <= player_chunk_z + 1
                && movedBlocks) {
                queue_chunk_for_sorting(mesh);
            }
        }
    }

    sort_chunk();
    load_chunk();
    if (movedChunks) {
        cam_cache.chunk_x = player_chunk_x;
        cam_cache.chunk_z = player_chunk_z;
    }

    // may want to preserve packet between frames to prevent the need for continuous quicksorting, would allow us to only update
    // if the camera has moved between chunk boundaries
    quicksort(packet, num_packets, sizeof(chunk_mesh*), chunk_distance_to_camera);

    for (int i = 0; i < num_packets; i++) {
        if (packet[i] == NULL) {
            continue;
        }
        render_sides(
            packet[i]->opaque_data,
            packet[i]->num_opaque_sides);
        render_sides(
            packet[i]->transparent_data,
            packet[i]->num_transparent_sides);
    }

    free(packet);
}