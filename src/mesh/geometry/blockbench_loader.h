#ifndef BLOCKBENCH_LOADER_H
#define BLOCKBENCH_LOADER_H

#include <stdbool.h>

// Single vertex with all data needed for rendering
typedef struct {
    float position[3];  // world position
    float normal[3];    // face normal
    float uv[2];        // texture coordinates (0.0-1.0)
} blockbench_vertex;

typedef struct {
    char* name;
    float from[3];
    float to[3];
    float rotation_angle;
    int rotation_axis;  // 0=x, 1=y, 2=z
    float rotation_origin[3];
    float face_uvs[6][4];  // [face][u1,v1,u2,v2] - north,east,south,west,up,down
    bool has_face[6];
} blockbench_element;

// Complete model ready for rendering
typedef struct {
    char* name;
    char* filepath;
    
    blockbench_element* elements;
    int element_count;

    float xz_orientation;
    
    // Texture information (atlas only)
    int texture_size[2]; // texture atlas size for UV mapping
    
    // Bounding box (useful for culling/collision)
    float bounds_min[3];
    float bounds_max[3];
} blockbench_model;



// Public API
void generate_element_vertices(blockbench_element* elem, int texture_size[2],
                               blockbench_vertex** vertices,
                               unsigned int** indices, int* vertex_count,
                               int* index_count);
blockbench_model* get_blockbench_model(const char* filepath);
blockbench_model copy_blockbench_model(const blockbench_model* model);
void free_blockbench_model(blockbench_model* model);
void blockbench_cleanup();

#endif
