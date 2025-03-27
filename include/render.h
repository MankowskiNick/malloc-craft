#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>
#include <block_renderer.h>
#include <skybox.h>
#include <sun.h>
#include <shadow_map.h>
#include <framebuffer.h>

typedef struct {
    block_renderer wr;
    block_renderer lr;
    skybox sky;
    sun s;
    framebuffer shadow_map;

    camera_cache cam_cache;
    camera* cam;
} renderer;

renderer create_renderer(camera* camera);
void destroy_renderer(renderer* r);
void render(renderer* r);

#endif