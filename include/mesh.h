#ifndef MESH_H
#define MESH_H

#include <chunk.h>
#include <hashmap.h>
#include <chunk_mesh.h>

#define VBO_WIDTH 7
#define VERTS_PER_SIDE 6
#define SIDE_OFFSET VERTS_PER_SIDE * VBO_WIDTH
#define SIDES_PER_CHUNK 1000

void m_init(camera* camera);
void m_cleanup();
chunk_mesh* update_chunk_mesh(int x, int z);
chunk_mesh* get_chunk_mesh(int x, int z);

#endif