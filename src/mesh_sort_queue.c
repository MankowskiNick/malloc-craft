#include <mesh_sort_queue.h>
#include <camera.h>
#include <mesh.h>
#include <stdlib.h>
#include <sort.h>

mesh_sort_queue* mesh_queue_head = NULL;
camera* m_cam_ref;

float* chunk_mesh_to_float_array(side_data* sides, int num_sides);

void mesh_sort_queue_init(camera* cam) {
    m_cam_ref = cam;
}

void mesh_sort_queue_cleanup() {
    mesh_sort_queue* cur = mesh_queue_head;
    while (cur != NULL) {
        mesh_sort_queue* next = cur->next;
        free(cur);
        cur = next;
    }
    mesh_queue_head = NULL;
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
        pow(sx - m_cam_ref->position[0], 2) +
        pow(sy - m_cam_ref->position[1], 2) +
        pow(sz - m_cam_ref->position[2], 2)
    );
}

void sort_transparent_sides(chunk_mesh* packet) {
    quicksort(packet->transparent_sides, packet->num_transparent_sides, sizeof(side_data), distance_to_camera);
    if (packet->transparent_data != NULL) {
        free(packet->transparent_data);
        packet->transparent_data = NULL;
    }
    packet->transparent_data = chunk_mesh_to_float_array(packet->transparent_sides, packet->num_transparent_sides);
}

void mesh_sort_queue_push(chunk_mesh* packet) {
    mesh_sort_queue* prev = NULL;
    mesh_sort_queue* cur = mesh_queue_head;
    int count = 0;
    while (cur != NULL) {
        count++;
        if (cur->x == packet->x && cur->z == packet->z) {
            cur->packet = packet;
            return;
        }
        
        prev = cur;
        cur = cur->next;
    }

    // Packet not in queue, add it
    mesh_sort_queue* new_node = malloc(sizeof(mesh_sort_queue));
    assert(new_node != NULL && "Failed to allocate memory for mesh queue node");
    new_node->packet = packet;
    new_node->x = packet->x;
    new_node->z = packet->z;
    new_node->next = NULL;

    if (prev == NULL) {
        mesh_queue_head = new_node;
    }
    else {
        prev->next = new_node;
    }
}

void mesh_sort_queue_remove(chunk_mesh* packet) {
    if (mesh_queue_head == NULL) {
        return;
    }

    mesh_sort_queue* prev = NULL;
    mesh_sort_queue* cur = mesh_queue_head;
    while (cur != NULL) {
        if (cur->x == packet->x && cur->z == packet->z) {
            if (prev == NULL) {
                mesh_queue_head = cur->next;
            }
            else {
                prev->next = cur->next;
            }
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void mesh_sort_queue_pop() {
    if (mesh_queue_head == NULL) {
        return;
    }

    mesh_sort_queue* cur = mesh_queue_head;
    mesh_queue_head = cur->next;
    sort_transparent_sides(cur->packet);
    free(cur);
}