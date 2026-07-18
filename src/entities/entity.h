#ifndef ENTITY_H
#define ENTITY_H

#include "../mesh/geometry/blockbench_loader.h"
#include "../util/game_clock.h" 

typedef enum {
	PLAYER
} entity_type;

typedef struct entity {
	blockbench_model model;
	float xz_orientation;
	float head_angle;

	bool is_walking;
	g_clock walk_animation_clock; 

	float x, y, z;
	unsigned int id;

	entity_type type;
} entity;

void update_model(entity* ent);
void pack_entity_vertices(entity* ent, float** custom_model_data,
                          int* num_custom_verts);

#endif 
