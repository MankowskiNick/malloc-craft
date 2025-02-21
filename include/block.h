#ifndef BLOCK_H
#define BLOCK_H

#include <block_types.h>
#include <camera.h>
#include <settings.h>

#define FRONT  0.0f
#define BACK   1.0f
#define LEFT   2.0f
#define RIGHT  3.0f
#define TOP    4.0f
#define BOTTOM 5.0f

typedef struct {
    int pos[3];
    block_type* type;
} block;

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
extern float CUBE_VERTICES[];

#endif