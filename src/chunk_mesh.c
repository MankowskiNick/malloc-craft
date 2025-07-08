#include <chunk_mesh.h>
#include <block.h>
#include <sort.h>
#include <chunk.h>
#include <mesh.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t cm_mutex = PTHREAD_MUTEX_INITIALIZER;

camera_cache cm_camera_cache = {0, 0, 0};

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

    // multiply by -1 to sort in descending order
    return -1.0f * sqrt(
        pow((float)(side->x) - cm_camera_cache.x, 2) +
        pow((float)(side->y) - cm_camera_cache.y, 2) +
        pow((float)(side->z) - cm_camera_cache.z, 2)
    );
}

void chunk_mesh_to_buffer(int* head, side_instance* sides, int num_sides) {
    // x y z type side
    for (int i = 0; i < num_sides; i++) {
        int index = i * VBO_WIDTH;
        side_instance side = sides[i];
        head[index + 0] = sides[i].x;
        head[index + 1] = sides[i].y;
        head[index + 2] = sides[i].z;
        head[index + 3] = sides[i].atlas_x;
        head[index + 4] = sides[i].atlas_y;
        head[index + 5] = sides[i].side;
        head[index + 6] = sides[i].underwater;
    }
    // return data;
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_instance), distance_to_camera);
}

void get_chunk_meshes(mesh_args* args) {

    int x = args->x;
    int z = args->z;

    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(x);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(z);

    int movedBlocks = ((int)x == (int)(cm_camera_cache.x) && (int)z == (int)(cm_camera_cache.z)) ? 0 : 1;

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

    quicksort(packet, count, sizeof(chunk_mesh*), distance_to_camera);

    lock_chunk_mesh();
    if (args->num_packets == NULL) {
        args->num_packets = malloc(sizeof(int));
    }
    *args->num_packets = count;
    args->packet = packet;
    unlock_chunk_mesh();

}

void update_chunk_meshes(mesh_args* args) {
    if (args == NULL) {
        assert(false && "mesh_args pointer is NULL\n");
    }
    get_chunk_meshes(args);

    usleep(TICK_RATE);
    update_chunk_meshes(args);
}

void lock_chunk_mesh() {
    pthread_mutex_lock(&cm_mutex);
}

void unlock_chunk_mesh() {
    pthread_mutex_unlock(&cm_mutex);
}

void start_chunk_mesh_updater(mesh_args* args) {
    pthread_t updater_thread;
    pthread_create(&updater_thread, NULL, (void* (*)(void*))update_chunk_meshes, args);
    pthread_detach(updater_thread);
}
