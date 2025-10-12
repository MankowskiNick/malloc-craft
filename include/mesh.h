#ifndef MESH_H
#define MESH_H

#include <chunk.h>
#include <hashmap.h>
#include <chunk_mesh.h>

#define SIDES_PER_CHUNK 1000
#define MODEL_VERTICES_PER_CHUNK 1000
#define FLOATS_PER_MODEL_VERT 8

void m_init(camera* camera);
void m_cleanup();
chunk_mesh* update_chunk_mesh(int x, int z);
chunk_mesh* get_chunk_mesh(int x, int z);
void queue_chunk_for_sorting(chunk_mesh* packet);
void sort_chunk();
void load_chunk();

#endif