#ifndef BUFFER_H
#define BUFFER_H

#include <util.h>

typedef struct {
    uint id;
    uint usage;
} VBO;

VBO create_vbo(uint usage);
void f_add_attrib(VBO* vbo, uint location, uint size, uint offset, uint stride);
void i_add_attrib(VBO* vbo, uint location, uint size, uint offset, uint stride);
void buffer_data(VBO vbo, uint usage, void* data, uint data_size);
void delete_vbo(VBO vbo);
void use_vbo(VBO vbo);

#endif