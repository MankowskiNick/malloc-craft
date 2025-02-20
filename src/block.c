#include <block.h>

int get_side_visible(uint side, block_type* blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE], int x, int y, int z) {
    uint visible = 0;
    switch(side) {
        case (int)TOP:
            if (y + 1 >= CHUNK_HEIGHT) {
                visible = 1;
            }
            else {
                block_type* b = blocks[x][y + 1][z];
                visible = blocks[x][y + 1][z] == NULL;
            }
            break;
        case (int)BOTTOM:
            if (y - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x][y - 1][z] == NULL;
            }
            break;
        case (int)BACK:
            if (x - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x - 1][y][z] == NULL;
            }
            break;
        case (int)FRONT:
            if (x + 1 >= CHUNK_SIZE) {
                visible = 1;
            }
            else {
                visible = blocks[x + 1][y][z] == NULL;
            }
            break;
        case (int)LEFT:
            if (z - 1 < 0) {
                visible = 1;
            }
            else {
                visible = blocks[x][y][z - 1] == NULL;
            }
            break;
        case (int)RIGHT:
            if (z + 1 >= CHUNK_SIZE) {
                visible = 1;
            }
            else {
                visible = blocks[x][y][z + 1] == NULL;
            }
            break;
        default:
            visible = 1;
    }
    return visible;
}


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
   -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, BOTTOM,

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
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, BACK
 };