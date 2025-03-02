#ifndef SKYBOX_H
#define SKYBOX_H

#include <vao.h>
#include <vbo.h>
#include <util.h>
#include <settings.h>
#include <shader.h>
#include <camera.h>
#include <texture.h>

typedef struct {
    VAO vao;
    VBO vbo;
    texture texture;
    float* vertices;
    float* texture_coords;
    int vertex_count;

    shader_program program;
} skybox;

skybox create_skybox();
void skybox_cleanup(skybox* s);
void render_skybox(skybox* s, camera* cam);

#endif