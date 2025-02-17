#ifndef VAO_H
#define VAO_H

#include <util.h>

typedef struct {
    uint id;
} VAO;

VAO create_vao();
void delete_vao(VAO vao);
void bind_vao(VAO vao);

#endif