#ifndef block_renderer_H
#define block_renderer_H
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
} block_renderer;

block_renderer create_block_renderer(camera* cam, char* atlas);
void destroy_block_renderer(block_renderer wr);

void render_solids(block_renderer* br, chunk_mesh** packet, int num_packets);
void render_transparent(block_renderer* br, chunk_mesh** packet, int num_packets);

#endif