#ifndef TEXTURE_H
#define TEXTURE_H

typedef struct {
    unsigned int id;
    int width, height;
    int u, v;
} atlas;

void t_init();
void t_cleanup();



#endif