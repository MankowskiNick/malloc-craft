#ifndef SHADOWMAP_H
#define SHADOWMAP_H

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
} shadow_map;

shadow_map create_shadow_map(uint width, uint height);
void shadow_map_cleanup(shadow_map* map);

void shadow_map_render(shadow_map* map, sun* s, world_mesh* packet);

void send_sun_matrices(shader_program* program, sun* sun);
void send_shadow_texture(shader_program* program, shadow_map* map);

#endif