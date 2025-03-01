#include <chunk_mesh.h>
#include <block.h>
#include <sort.h>
#include <chunk.h>
#include <assert.h>


camera* cm_cam_ref;

void chunk_mesh_init(camera* camera) {
    cm_cam_ref = camera;
}

int chunk_coord_equals(void* a, void* b) {
    chunk_coord* c1 = (chunk_coord*)a;
    chunk_coord* c2 = (chunk_coord*)b;

    return c1->x == c2->x && c1->z == c2->z;
}

int chunk_mesh_equals(void* a, void* b) {
    chunk_mesh* c1 = (chunk_mesh*)a;
    chunk_mesh* c2 = (chunk_mesh*)b;
    return c1->x == c2->x && c2->z == c2->z;
}

float distance_to_camera(const void* item) {
    side_data* side = (side_data*)item;

    float sx = 0.0f;
    float sy = 0.0f;
    float sz = 0.0f;
    for (int i = 0; i < VERTS_PER_SIDE; i++) {
        sx += side->vertices[i].x;
        sy += side->vertices[i].y;
        sz += side->vertices[i].z;
    }
    sx /= VERTS_PER_SIDE;
    sy /= VERTS_PER_SIDE;
    sz /= VERTS_PER_SIDE;

    // multiply by -1 to sort in descending order
    return -1.0f * sqrt(
        pow(sx - cm_cam_ref->position[0], 2) +
        pow(sy - cm_cam_ref->position[1], 2) +
        pow(sz - cm_cam_ref->position[2], 2)
    );
}

float* chunk_mesh_to_float_array(side_data* sides, int num_sides) {
    float* data = malloc(num_sides * SIDE_OFFSET * sizeof(float));
    assert(data != NULL && "Failed to allocate memory for float array");

    for (int i = 0; i < num_sides; i++) {
        for (int j = 0; j < VERTS_PER_SIDE; j++) {
            int index = i * SIDE_OFFSET + j * VBO_WIDTH;
            data[index + 0] = sides[i].vertices[j].x;
            data[index + 1] = sides[i].vertices[j].y;
            data[index + 2] = sides[i].vertices[j].z;
            data[index + 3] = sides[i].vertices[j].tx;
            data[index + 4] = sides[i].vertices[j].ty;
            data[index + 5] = sides[i].vertices[j].atlas_x;
            data[index + 6] = sides[i].vertices[j].atlas_y;
        }
    }
    return data;
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_data), distance_to_camera);
    if (packet->transparent_data != NULL) {
        free(packet->transparent_data);
        packet->transparent_data = NULL;
    }
    packet->transparent_data = chunk_mesh_to_float_array(packet->transparent_sides, packet->num_transparent_sides);
}