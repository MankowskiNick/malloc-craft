#include <block.h>

float CUBE_VERTICES[] = {

    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, FRONT,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, FRONT,
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f, FRONT,
    0.5f,  0.5f,  0.5f,  0.0f, 1.0f, FRONT,
 
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,
     -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, BACK,
     -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
     -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, BACK,
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BACK,
     -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK,

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


    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, TOP,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, TOP,
   -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, TOP,
   -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, TOP,
   
   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM,
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f, BOTTOM,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, BOTTOM,
   -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, BOTTOM,
   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM
 };