#ifndef BLOCK_H
#define BLOCK_H

#include <block_models.h>
#include <camera.h>
#include <settings.h>
#include <util.h>

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
extern float CUBE_VERTICES[];

void break_block(camera cam);
void place_block(camera cam);

block_type* get_block_type(uint id);

#endif