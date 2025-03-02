#ifndef SKYBOX_H
#define SKYBOX_H

#include <vao.h>
#include <vbo.h>
#include <util.h>
#include <settings.h>
#include <shader.h>
#include <camera.h>

typedef struct {
    VAO vao;
    VBO vbo;
    uint texture;
    float* vertices;
    int vertex_count;

    shader_program program;
} skybox;

skybox* create_skybox();
void destroy_skybox(skybox* s);
void draw_skybox(skybox* s, camera* cam);

#endif