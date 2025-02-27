#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>
#include <mesh.h>

void render_cube();
void r_init(shader_program* program, camera* camera);
void r_cleanup();
void render(camera cam, shader_program program);

void update_sorting_queue(int x, int z, chunk_mesh* packet);

#endif