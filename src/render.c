#include <render.h>

#include <block_renderer.h>
#include <liquid_renderer.h>
#include <skybox.h>
#include <glad/glad.h>
#include <chunk_mesh.h>
#include <mesh.h>
#include <sort.h>
#include <world.h>
#include <util.h>
#include <settings.h>
#include <world_mesh.h>

#define CAMERA_POS_TO_CHUNK_POS(x) x >= 0 ? (int)(x / CHUNK_SIZE) : (int)(x / CHUNK_SIZE) - 1

camera* r_cam_ref;

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

renderer create_renderer(camera* camera) {
    r_cam_ref = camera;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    #ifdef WIREFRAME
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    #endif

    glViewport(0, 0, WIDTH, HEIGHT);

    w_init();
    m_init(camera);

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = (int)(camera->position[0]) - CHUNK_RENDER_DISTANCE + i;
            int z = (int)(camera->position[2]) - CHUNK_RENDER_DISTANCE + j;
            get_chunk_mesh(x, z);
            load_chunk();
        }
    }

    block_renderer wr = create_block_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    block_renderer lr = create_liquid_renderer(camera, ATLAS_PATH, BUMP_PATH, CAUSTIC_PATH);
    skybox sky = create_skybox(camera);
    sun s = create_sun(camera, 1.0f, 1.0f, 1.0f);
    framebuffer map = create_shadow_map(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);

    renderer r = {
        .wr = wr,
        .lr = lr,
        .sky = sky,
        .s = s,
        .cam_cache = {
            .x = camera->position[0],
            .z = camera->position[2],
        },
        .cam = camera,
        .shadow_map = map,
    };

    return r;
}

void destroy_renderer(renderer* r) {
    m_cleanup();
    destroy_block_renderer(r->wr);
    destroy_block_renderer(r->lr);
    skybox_cleanup(&(r->sky));
    // sun_cleanup(&(r->s));
}

world_mesh* get_world_mesh(renderer* r, int* num_packets) {
    int x = r->cam->position[0];
    int z = r->cam->position[2];
    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(x);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(z);

    int movedBlocks = ((int)x == (int)r->cam_cache.x && (int)z == (int)r->cam_cache.z) ? 0 : 1;
    if (movedBlocks) {
        r->cam_cache.x = x;
        r->cam_cache.z = z;
    }

    chunk_mesh** packet = NULL;
    int count = 0;

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = player_chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = player_chunk_z - CHUNK_RENDER_DISTANCE + j;

            if (sqrt(pow(x - player_chunk_x, 2) + pow(z - player_chunk_z, 2)) > CHUNK_RENDER_DISTANCE) {
                continue;
            }

            chunk_mesh* mesh = get_chunk_mesh(x, z);

            if (mesh == NULL) {
                continue;
            }

            packet = realloc(packet, (count + 1) * sizeof(chunk_mesh*));
            packet[count] = mesh;
            count++;

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

    quicksort(packet, count, sizeof(chunk_mesh*), chunk_distance_to_camera);

    *num_packets = count;

    world_mesh* world = create_world_mesh(packet, count);
    if (!world) {
        assert(false && "Failed to create world mesh\n");
    }

    // Free the original chunk meshes
    free(packet);
    
    return world;
}

void render(renderer* r) {
    int num_packets = 0;
    world_mesh* packet = get_world_mesh(r, &num_packets);

    shadow_map_render(&(r->shadow_map), &(r->s), packet);
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_2D, r->shadow_map.texture);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    render_skybox(&(r->sky));
    glEnable(GL_DEPTH_TEST);

    render_sun(&(r->s));

    glClear(GL_DEPTH_BUFFER_BIT);
    
    render_solids(&(r->wr), &(r->s), &(r->shadow_map), packet);

    render_liquids(&(r->lr), &(r->s), &(r->shadow_map), packet);

    render_transparent(&(r->wr), &(r->s), &(r->shadow_map), packet);

    free(packet->transparent_data);
    free(packet->opaque_data);
    free(packet->liquid_data);
    free(packet);
}