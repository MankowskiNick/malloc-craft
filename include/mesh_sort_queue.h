#ifndef MESH_SORT_QUEUE_H
#define MESH_SORT_QUEUE_H

#include <chunk_mesh.h>
#include <camera.h>

typedef struct mesh_sort_queue {
    int x, z;
    chunk_mesh* packet;
    struct mesh_sort_queue* next;
} mesh_sort_queue;

void mesh_sort_queue_init(camera* camera);
void mesh_sort_queue_push(chunk_mesh* packet);
void mesh_sort_queue_remove(chunk_mesh* packet);
void mesh_sort_queue_pop();

#endif