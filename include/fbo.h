#ifndef FBO_H
#define FBO_H

#include <util.h>
#include <chunk_mesh.h>
#include <shader.h>
#include <vao.h>
#include <vbo.h>
#include <sun.h>
#include <world_mesh.h>
#include <settings.h>

typedef struct {
    uint width;
    uint height;
    uint fbo;
    uint texture;
    shader_program program;
    VAO vao;
    VBO cube_vbo, instance_vbo;
} FBO;

FBO create_reflection_map(uint width, uint height);

void FBO_cleanup(FBO* map);

void FBO_render(FBO* map, sun* s, world_mesh* packet);

void send_sun_matrices(shader_program* program, sun* sun);
void send_fbo_texture(shader_program* program, FBO* map, uint texture_index, char* uniform_name);

#endif