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
    return c1->x == c2->x && c1->z == c2->z;
}

float distance_to_camera(const void* item) {
    side_instance* side = (side_instance*)item;

    // multiply by -1 to sort in descending order
    return -1.0f * sqrt(
        pow((float)(side->x) - cm_cam_ref->position[0], 2) +
        pow((float)(side->y) - cm_cam_ref->position[1], 2) +
        pow((float)(side->z) - cm_cam_ref->position[2], 2)
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