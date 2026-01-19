#ifndef BLOCK_H
#define BLOCK_H

#include <block_models.h>
#include <player/core/player.h>
#include <util/settings.h>
#include <vao.h>
#include <vbo.h>
#include <game_data.h>
#include <util.h>

#define VERTS_PER_SIDE 6

// Vertex data for a cube
// v.x v.y v.z t.x t.y side
#define CUBE_VERTICES_WIDTH 6
extern float CUBE_VERTICES[];

void block_init();

void break_block(game_data* data);
void place_block(game_data* data);
void update_selected_block(player* p);
short get_block_id(char* name);
bool check_block_foliage(short id);

void send_cube_vbo(VAO vao, VBO vbo);

block_type* get_block_type(short id);

#endif
