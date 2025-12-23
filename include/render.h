#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>
#include <block_renderer.h>
#include <blockbench_renderer.h>
#include <skybox.h>
#include <sun.h>
#include <fbo.h>
#include <reflection_map.h>
#include <game_data.h>

typedef struct {
    block_renderer wr; // world renderer
    block_renderer fr; // foliage renderer
    block_renderer lr; // liquid renderer
    blockbench_renderer br; // blockbench renderer
    skybox sky;
    sun s;
    FBO shadow_map;
    FBO reflection_map;

    camera_cache cam_cache;
    camera* cam;
} renderer;

typedef struct { 
    renderer* r;
    world_mesh* packet;
    int num_packets;
} render_args;

renderer create_renderer(camera* camera);
void destroy_renderer(renderer* r);
void render(game_data* args, renderer* r, world_mesh* packet, int num_packets);

#endif
