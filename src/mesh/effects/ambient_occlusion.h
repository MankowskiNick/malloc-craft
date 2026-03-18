#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include <chunk.h>

// Calculate AO for all 4 vertices of a face.
// Returns packed int with 4 AO values (2 bits each): v0 | (v1 << 2) | (v2 << 4) | (v3 << 6)
int calculate_face_ao(int x, int y, int z, int face, chunk* c, chunk* adj_chunks[4]);

#endif
