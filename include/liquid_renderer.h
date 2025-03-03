#ifndef LIQUID_RENDERER
#define LIQUID_RENDERER

#include <block_renderer.h>

block_renderer create_liquid_renderer(camera* cam, char* atlas);

void render_liquids(block_renderer* br, chunk_mesh** packet, int num_packets);

#endif