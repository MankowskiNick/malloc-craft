#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <util.h>
#include <chunk_mesh.h>
#include <shader.h>
#include <vao.h>
#include <vbo.h>
#include <sun.h>
#include <framebuffer.h>
#include <world_mesh.h>
#include <settings.h>

framebuffer create_shadow_map(uint width, uint height);
void shadow_map_cleanup(framebuffer* map);

void shadow_map_render(framebuffer* map, sun* s, world_mesh* packet);

void send_sun_matrices(shader_program* program, sun* sun);
void send_shadow_texture(shader_program* program, framebuffer* map);

#endif