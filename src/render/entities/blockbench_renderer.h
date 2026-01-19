#ifndef BLOCKBENCH_RENDERER_H
#define BLOCKBENCH_RENDERER_H

#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <camera.h>
#include <texture.h>
#include <sun.h>
#include <fbo.h>
#include <world_mesh.h>

typedef struct {
    VAO vao;
    VBO vertex_vbo;
    shader_program program;
    camera* cam;
    texture atlas;
    texture bump;
} blockbench_renderer;

blockbench_renderer create_blockbench_renderer(camera* cam, char* atlas, char* bump);
void destroy_blockbench_renderer(blockbench_renderer br);

void send_blockbench_view_matrix(blockbench_renderer* br);
void send_blockbench_proj_matrix(blockbench_renderer* br);
void send_blockbench_atlas(blockbench_renderer* br);
void send_blockbench_fog(blockbench_renderer* br);
void send_blockbench_time(blockbench_renderer* br);

void render_blockbench_models(blockbench_renderer* br, sun* sun, FBO* shadow_map, world_mesh* packet);

#endif
