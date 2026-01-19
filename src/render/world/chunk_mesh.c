#include "chunk_mesh.h"
#include "block.h"
#include "sort.h"
#include "chunk.h"
#include "mesh.h"
#include "settings.h"
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "water.h"

pthread_mutex_t cm_mutex = PTHREAD_MUTEX_INITIALIZER;

camera_cache cm_camera_cache = {0, 0, 0};

void lock_mesh() {
    pthread_mutex_lock(&cm_mutex);
}

void unlock_mesh() {
    pthread_mutex_unlock(&cm_mutex);
}

void chunk_mesh_init(camera* camera) {
    cm_camera_cache.x = camera->position[0];
    cm_camera_cache.y = camera->position[1];
    cm_camera_cache.z = camera->position[2];
}

int chunk_coord_equals(void* a, void* b) {
    chunk_coord* c1 = (chunk_coord*)a;
    chunk_coord* c2 = (chunk_coord*)b;

    return c1->x == c2->x && c1->z == c2->z;
}

int chunk_mesh_equals(void* a, void* b) {
    chunk_mesh* c1 = (chunk_mesh*)a;
    chunk_mesh* c2 = (chunk_mesh*)b;
    return c1->x == c2->x && c1->z == c2->z;
}

float distance_to_camera(const void* item) {
    side_instance* side = (side_instance*)item;

    // multiply by -1 to sort in descending order (back-to-front)
    return -1.0f * sqrt(
        pow((float)(side->x) - cm_camera_cache.x, 2) +
        pow((float)(side->y) - cm_camera_cache.y, 2) +
        pow((float)(side->z) - cm_camera_cache.z, 2)
    );
}

float liquid_distance_to_camera(const void* item) {
    side_instance* side = (side_instance*)item;

    float dist = sqrt(
        pow((float)(side->x) - cm_camera_cache.x, 2) +
        pow((float)(side->y) - cm_camera_cache.y, 2) +
        pow((float)(side->z) - cm_camera_cache.z, 2)
    );

    // Above water: sort front-to-back (positive) so closer water renders first
    // Below water: sort back-to-front (negative) so distant water renders first
    if (cm_camera_cache.y > (float)(WORLDGEN_WATER_LEVEL + 1)) {
        return dist;  // front-to-back
    } else {
        return -dist; // back-to-front
    }
}

float chunk_distance_to_camera(const void* item) {
    chunk_mesh** mesh_ptr = (chunk_mesh**)item;
    chunk_mesh* mesh = *mesh_ptr;

    // Calculate chunk center position
    float chunk_center_x = (float)(mesh->x * CHUNK_SIZE) + (CHUNK_SIZE / 2.0f);
    float chunk_center_z = (float)(mesh->z * CHUNK_SIZE) + (CHUNK_SIZE / 2.0f);

    // multiply by -1 to sort in descending order (back-to-front)
    return -1.0f * sqrt(
        pow(chunk_center_x - cm_camera_cache.x, 2) +
        pow(chunk_center_z - cm_camera_cache.z, 2)
    );
}

void chunk_mesh_to_buffer(int* head, side_instance* sides, int num_sides) {
    // x y z type side
    for (int i = 0; i < num_sides; i++) {
        int index = i * VBO_WIDTH;
        side_instance side = sides[i];
        head[index + 0] = side.x;
        head[index + 1] = side.y;
        head[index + 2] = side.z;
        head[index + 3] = side.atlas_x;
        head[index + 4] = side.atlas_y;
        head[index + 5] = side.side;
        head[index + 6] = side.underwater;
        head[index + 7] = side.orientation;
        head[index + 8] = side.water_level;
        head[index + 9] = side.water_level_transition;
    }
}

void custom_vert_to_buffer(float* head, float* custom_verts, int num_custom_verts) {
    for (int i = 0; i < num_custom_verts; i++) {
        int index = i * FLOATS_PER_MODEL_VERT;

        for (int j = 0; j < FLOATS_PER_MODEL_VERT; j++) {
            head[index + j] = custom_verts[index + j];
        }
    }
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_instance), distance_to_camera);
}

void sort_liquid_sides(chunk_mesh* packet) {
    quicksort(packet->liquid_sides, packet->num_liquid_sides, sizeof(side_instance), liquid_distance_to_camera);
}

void get_chunk_meshes(game_data* args) {
    int x = args->x;
    int z = args->z;

    int player_chunk_x = WORLD_POS_TO_CHUNK_POS(x);
    int player_chunk_z = WORLD_POS_TO_CHUNK_POS(z);

    int movedBlocks = ((int)x == (int)(cm_camera_cache.x) && (int)z == (int)(cm_camera_cache.z)) ? 0 : 1;

    // Update camera cache for sorting
    cm_camera_cache.x = args->x;
    cm_camera_cache.y = args->player->cam.position[1];
    cm_camera_cache.z = args->z;

    chunk_mesh** packet = NULL;
    int count = 0;

    for (int i = 0; i < 2 * CHUNK_RENDER_DISTANCE; i++) {
        for (int j = 0; j < 2 * CHUNK_RENDER_DISTANCE; j++) {
            int x = player_chunk_x - CHUNK_RENDER_DISTANCE + i;
            int z = player_chunk_z - CHUNK_RENDER_DISTANCE + j;

            if (sqrt(pow(x - player_chunk_x, 2) + pow(z - player_chunk_z, 2)) > CHUNK_RENDER_DISTANCE) {
                continue;
            }
            
            lock_mesh();
            chunk_mesh* mesh = get_chunk_mesh(x, z);
            unlock_mesh();

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

    lock_mesh();
    if (args->num_packets == NULL) {
        args->num_packets = malloc(sizeof(int));
    }
    *args->num_packets = count;
    args->packet = packet;
    unlock_mesh();

}

void update_chunk_meshes(game_data* data) {
    if (data == NULL) {
        assert(false && "game_data pointer is NULL\n");
    }

    while(data->is_running) {
        get_chunk_meshes(data);
        flood_chunks(data);
        usleep(TICK_RATE);
    }

    return;
}

void start_chunk_mesh_updater(game_data* data) {
    pthread_t updater_thread;
    if (data == NULL) {
        assert(false && "game_data pointer is NULL\n");
    }
    pthread_create(&updater_thread, NULL, (void* (*)(void*))update_chunk_meshes, data);
    pthread_detach(updater_thread);
}
