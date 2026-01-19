#include "vao.h"
#include <glad/glad.h>

VAO create_vao() {
    uint id;
    glGenVertexArrays(1, &id);
    VAO vao = {
        .id = id
    };

    return vao;
}

void delete_vao(VAO vao) {
    glDeleteVertexArrays(1, &vao.id);
}

void bind_vao(VAO vao) {
    glBindVertexArray(vao.id);
}