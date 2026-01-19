#ifndef OUTLINE_RENDERER_H
#define OUTLINE_RENDERER_H

#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <player/core/camera.h>
#include <cglm/cglm.h>

typedef struct {
    VAO vao;
    VBO vbo;
    shader_program program;
    camera* cam;
} outline_renderer;

outline_renderer create_outline_renderer(camera* cam);
void destroy_outline_renderer(outline_renderer or);

void send_outline_view_matrix(outline_renderer* or);
void send_outline_proj_matrix(outline_renderer* or);
void render_outline(outline_renderer* or, int block_x, int block_y, int block_z);

#endif
