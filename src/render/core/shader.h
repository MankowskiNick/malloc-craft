#ifndef SHADER_H
#define SHADER_H

#include <util.h>

typedef struct {
    uint id;
    uint type;
} shader;

typedef struct {
    uint id;
    shader* vert_shader, frag_shader;
} shader_program;

shader create_shader(const char* source, uint type);
void delete_shader(shader shader);

shader_program create_program(shader vert_shader, shader frag_shader);
void delete_program(shader_program program);
void use_program(shader_program program);
void stop_program();

#endif