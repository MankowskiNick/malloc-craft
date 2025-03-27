#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

typedef struct {
    uint width;
    uint height;
    uint fbo;
    uint texture;
    shader_program program;
    VAO vao;
    VBO cube_vbo, instance_vbo;
} framebuffer;

void framebuffer_cleanup(framebuffer* map);

#endif