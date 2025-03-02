#ifndef WORLD_RENDERER_H
#define WORLD_RENDERER_H
#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <camera.h>
#include <texture.h>

typedef struct {
    float x, z;
} camera_cache;

typedef struct {
    VAO vao;
    VBO cube_vbo, instance_vbo;
    shader_program program;
    camera* cam;
    texture atlas;


    camera_cache cam_cache;
} world_renderer;

world_renderer create_world_renderer(camera* cam, char* atlas);
void destroy_world_renderer(world_renderer wr);

void render_world(world_renderer* wr);

#endif