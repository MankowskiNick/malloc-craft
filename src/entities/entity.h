#ifndef ENTITY_H
#define ENTITY_H

#include "../mesh/geometry/blockbench_loader.h"

typedef struct entity {
	blockbench_model model;
	float xz_orientation;
	float head_angle;

	float x, y, z;
	unsigned int id;
} entity;

void update_model(entity* ent);
void pack_entity_vertices(entity* ent, float** custom_model_data,
                          int* num_custom_verts);

#endif 
