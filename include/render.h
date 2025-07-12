#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>
#include <block_renderer.h>
#include <skybox.h>
#include <sun.h>
#include <shadow_map.h>

typedef struct {
    block_renderer wr;
    block_renderer lr;
    skybox sky;
    sun s;
    shadow_map map;

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
void render(renderer* r, world_mesh* packet, int num_packets);

#endif