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
    texture caustic;
} block_renderer;

block_renderer create_block_renderer(camera* cam, char* atlas, char* caustic);
void destroy_block_renderer(block_renderer wr);

void send_view_matrix(block_renderer* br);
void send_proj_matrix(block_renderer* br);
void send_atlas(block_renderer* br);
void send_fog(block_renderer* br);

void render_sides(block_renderer* br, int* side_data, int num_sides);

void render_solids(block_renderer* br, chunk_mesh** packet, int num_packets);
void render_transparent(block_renderer* br, chunk_mesh** packet, int num_packets);

#endif