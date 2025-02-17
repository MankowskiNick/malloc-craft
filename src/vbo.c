#include <vbo.h>
#include <glad/glad.h>
#include <stdio.h>

VBO create_vbo(int location, int size, int stride, int offset) {
    uint id;
    glGenBuffers(1, &id);
    VBO vbo = {
        .id = id,
        .location = location,
        .size = size,
        .stride = stride,
        .offset = offset
    };

    return vbo;
}

void use_vbo(VBO vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
    glVertexAttribPointer(vbo.location, 
                            vbo.size,
                            GL_FLOAT,
                            GL_FALSE, 
                            vbo.stride, 
                            (void*)(vbo.offset));
    glEnableVertexAttribArray(vbo.location);
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