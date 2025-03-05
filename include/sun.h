#ifndef SUN_H
#define SUN_H

#include <vao.h>
#include <vbo.h>
#include <shader.h>
#include <camera.h>

typedef struct {
    float x, y, z;
    float r, g, b;

    camera* cam;

    VAO vao;
    VBO vbo;
    shader_program program;
    float* vertices;
    int vertex_count;
} sun;

sun create_sun(camera* cam, float r, float g, float b);
void sun_cleanup(sun* s);

void send_sun_info(shader_program* p, sun* s);

void update_sun(sun* s, float t);
void render_sun(sun* s);

#endif