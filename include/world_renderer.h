#ifndef WORLD_RENDERER_H
#define WORLD_RENDERER_H
#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <camera.h>
#include <texture.h>
#include <chunk_mesh.h>

typedef struct {
    VAO vao;
    VBO cube_vbo, instance_vbo;
    shader_program program;
    camera* cam;
    texture atlas;
} world_renderer;

world_renderer create_world_renderer(camera* cam, char* atlas);
void destroy_world_renderer(world_renderer wr);

void render_world(world_renderer* wr, chunk_mesh** packet, int num_packets);

#endif