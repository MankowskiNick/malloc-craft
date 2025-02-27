#ifndef RENDER_H
#define RENDER_H

#include <camera.h>
#include <shader.h>

void render_cube();
void r_init(shader_program* program);
void r_cleanup();
void render(camera cam, shader_program program);

#endif