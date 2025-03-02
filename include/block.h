#ifndef BLOCK_H
#define BLOCK_H

#include <block_models.h>
#include <player_instance.h>
#include <settings.h>
#include <util.h>

#define VERTS_PER_SIDE 6

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
extern float CUBE_VERTICES[];

void break_block(player_instance player);
void place_block(player_instance player);

block_type* get_block_type(short id);

#endif