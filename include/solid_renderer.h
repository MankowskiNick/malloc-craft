#ifndef solid_renderer_H
#define solid_renderer_H
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
} solid_renderer;

solid_renderer create_solid_renderer(camera* cam, char* atlas);
void destroy_solid_renderer(solid_renderer wr);

void render_solids(solid_renderer* wr, chunk_mesh** packet, int num_packets);

#endif