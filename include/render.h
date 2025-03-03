#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>
#include <world_renderer.h>
#include <skybox.h>

typedef struct {
    world_renderer wr;
    skybox sky;
    camera_cache cam_cache;
    camera* cam;
} renderer;

renderer create_renderer(camera* camera);
void destroy_renderer(renderer* r);
void render(renderer* r);

#endif