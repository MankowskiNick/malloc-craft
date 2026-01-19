#ifndef WATER_H
#define WATER_H

#include <chunk.h>
#include <game_data.h>
#include <util.h>

void flood_chunks(game_data* data);
void check_for_flow(game_data* data, chunk* c, chunk* adj[4], int x, int y, int z);

#endif
