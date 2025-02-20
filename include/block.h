#ifndef BLOCK_H
#define BLOCK_H

#define LEFT   0.0f
#define RIGHT  1.0f
#define TOP    2.0f
#define BOTTOM 3.0f
#define FRONT  4.0f
#define BACK   5.0f

typedef struct {
    uint id;
    float face_atlas_coords[6][2];
} block;

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
float CUBE_VERTICES[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, LEFT,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, LEFT,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, LEFT,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, LEFT,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, LEFT,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, LEFT,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, RIGHT,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, RIGHT,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, RIGHT,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, RIGHT,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, RIGHT,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, RIGHT,

    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, BACK,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BACK,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,

     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, FRONT,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, FRONT,
     0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f, BOTTOM,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BOTTOM,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, TOP,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, TOP,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP
};


#endif