#ifndef BUFFER_H
#define BUFFER_H

#include <util.h>
#include <glad/glad.h>

typedef struct {
    uint id;
    uint location;
    uint size;
    uint offset;
    uint stride;
} VBO;

VBO create_vbo(int location, int size, int stride, int offset);
void buffer_data(VBO* vbo, uint usage, void* data, uint data_size);
void delete_vbo(VBO* vbo);
void use_vbo(VBO* vbo);

#endif