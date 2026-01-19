#ifndef SKYBOX_H
#define SKYBOX_H

#include <vao.h>
#include <vbo.h>
#include <util.h>
#include <settings.h>
#include <shader.h>
#include <player/core/camera.h>
#include <texture.h>

typedef struct {
    VAO vao;
    VBO vbo;
    texture texture;
    float* vertices;
    float* texture_coords;
    int vertex_count;
    camera* cam;

    shader_program program;
} skybox;

skybox create_skybox(camera* cam);
void skybox_cleanup(skybox* s);
void render_skybox(skybox* s);
void send_sky_view_matrix(shader_program* p, camera* cam);
void send_sky_proj_matrix(shader_program* p);

#endif