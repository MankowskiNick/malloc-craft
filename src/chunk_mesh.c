#include <chunk_mesh.h>
#include <block.h>
#include <sort.h>
#include <chunk.h>
#include <assert.h>
#include <pthread.h>
#include <mesh.h>


camera_cache cm_cam_ref;
chunk_mesh* cm_cached_mesh = NULL;
pthread_mutex_t cm_lock;

void chunk_mesh_init(camera* camera) {
    if (camera == NULL) {
        assert(false && "Camera pointer is NULL\n");
    }
    pthread_mutex_lock(&cm_lock);
    cm_cam_ref.x = camera->position[0];
    cm_cam_ref.z = camera->position[2];
    cm_cached_mesh = NULL;
    pthread_mutex_unlock(&cm_lock);
    pthread_mutex_init(&cm_lock, NULL);
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
        pow((float)(side->x) - cm_cam_ref.x, 2) +
        pow((float)(side->y) - cm_cam_ref.y, 2) +
        pow((float)(side->z) - cm_cam_ref.z, 2)
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

float chunk_distance_to_camera(const void* item) {
    chunk_mesh* packet = *(chunk_mesh**)item;
    // camera coords to chunk coords
    float x = (cm_cam_ref.x / (float)CHUNK_SIZE);
    float z = (cm_cam_ref.z / (float)CHUNK_SIZE);

    return -1.0f * sqrt(
        pow((float)(packet->x + 0.5f) - x, 2) +
        pow((float)(packet->z + 0.5f) - z, 2)
    );
}

chunk_mesh** get_chunk_meshes(chunk_args* args) {
    int* num_packets = args->num_packets;
    if (num_packets == NULL) {
        assert(false && "num_packets pointer is NULL\n");
    }
    int x = args->x;
    int z = args->z;

    int player_chunk_x = CAMERA_POS_TO_CHUNK_POS(x);
    int player_chunk_z = CAMERA_POS_TO_CHUNK_POS(z);

    int movedBlocks;
    pthread_mutex_lock(&cm_lock);
    movedBlocks = ((int)x == (int)(cm_cam_ref.x) && (int)z == (int)(cm_cam_ref.z)) ? 0 : 1;
    if (movedBlocks) {
        cm_cam_ref.x = x;
        cm_cam_ref.z = z;
    }
    pthread_mutex_unlock(&cm_lock);

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

    return packet;
}
