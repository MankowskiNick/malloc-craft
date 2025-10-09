#ifndef BLOCKBENCH_LOADER_H
#define BLOCKBENCH_LOADER_H

#include <stdlib.h>
#include <stdbool.h>

// Single vertex with all data needed for rendering
typedef struct {
    float position[3];  // world position
    float normal[3];    // face normal
    float uv[2];        // texture coordinates (0.0-1.0)
} blockbench_vertex;

// Complete model ready for rendering
typedef struct {
    char* name;
    char* filepath;
    
    // Pre-computed vertex data (ready for GPU)
    blockbench_vertex* vertices;
    unsigned int* indices;
    int vertex_count;
    int index_count;
    
    // Texture information (atlas only)
    int texture_size[2]; // texture atlas size for UV mapping
    
    // Bounding box (useful for culling/collision)
    float bounds_min[3];
    float bounds_max[3];
} blockbench_model;

// Public API
blockbench_model* get_blockbench_model(const char* filepath);
void blockbench_cleanup();

#endif
