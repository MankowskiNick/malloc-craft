#include "vbo.h"
#include <glad/glad.h>
#include <stdio.h>

VBO create_vbo(uint usage) {
    uint id;
    glGenBuffers(1, &id);
    VBO vbo = {
        .id = id,
        .usage = usage
    };

    return vbo;
}

void use_vbo(VBO vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
}

void f_add_attrib(VBO* vbo, uint location, uint size, uint offset, uint stride) {
    glVertexAttribPointer(location, 
                            size,
                            GL_FLOAT,
                            GL_FALSE, 
                            stride, 
                            (void*)(offset));
    glEnableVertexAttribArray(location);
}

void i_add_attrib(VBO* vbo, uint location, uint size, uint offset, uint stride) {
    glVertexAttribIPointer(location, 
                            size,
                            GL_INT,
                            stride, 
                            (void*)(offset));
    glEnableVertexAttribArray(location);
}

void buffer_data(VBO vbo, uint usage, void* data, uint data_size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
    glBufferData(GL_ARRAY_BUFFER, 
                    data_size,
                    data,
                    usage);
}

void delete_vbo(VBO vbo) {
    glDeleteBuffers(1, &(vbo.id));
}