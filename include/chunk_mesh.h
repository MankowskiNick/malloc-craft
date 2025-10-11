#ifndef CHUNK_MESH_H
#define CHUNK_MESH_H

#include <camera.h>
#include <player_instance.h>

#define VBO_WIDTH 7

typedef struct {
    int x, y, z;
    short atlas_x, atlas_y;
    short side;
    short face;
    short underwater;
    short orientation;
} side_instance;

typedef struct {
    int x, z;
    side_instance* opaque_sides;
    side_instance* liquid_sides;
    side_instance* transparent_sides;
    side_instance* foliage_sides;
    float* custom_model_data;
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;
    int num_foliage_sides;
    int num_custom_verts;
} chunk_mesh;

typedef struct {
    int num_opaque_sides;
    int num_transparent_sides;
    int num_liquid_sides;
    int num_foliage_sides;
    int num_custom_verts;

    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
    int* foliage_data;
    float* custom_model_data;
} world_mesh;

typedef struct {
    int x;
    int z;
    int* num_packets;
    chunk_mesh** packet;
    world_mesh* world_mesh;
    player_instance* player;
    int is_running;
    int mesh_requires_update;
} game_data;

void chunk_mesh_init(camera* camera);
int chunk_mesh_equals(void* a, void* b);
void chunk_mesh_to_buffer(int* head, side_instance* sides, int num_sides);
void custom_vert_to_buffer(float* head, float* custom_verts, int num_custom_verts);
void sort_transparent_sides(chunk_mesh* packet);
void get_chunk_meshes(game_data* args);
void start_chunk_mesh_updater(game_data* args);

void lock_mesh();
void unlock_mesh();

#endif
