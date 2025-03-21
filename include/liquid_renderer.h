#ifndef LIQUID_RENDERER
#define LIQUID_RENDERER

#include <block_renderer.h>
#include <world_mesh.h>

block_renderer create_liquid_renderer(camera* cam, char* atlas, char* bump, char* caustic);

void render_liquids(block_renderer* br, sun* sun, shadow_map* map, world_mesh* packet);

#endif