#include "entity.h"

#include <assert.h>
#include <cglm/cglm.h>
#include <game_data.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

void free_entity(entity* ent) {
	free(ent);
}

void update_head_orientation(entity* ent) {
	blockbench_element* head = NULL;
	for (int i = 0; i < ent->model.element_count; i++) {
		if (strcmp(ent->model.elements[i].name, "head") == 0) {
			head = &(ent->model.elements[i]);
		}
	}

	if (head == NULL) {
		return;
	}

	// head rotates w.r.t. z axis to look up and down
	head->rotation_axis = 2;
	head->rotation_angle = ent->head_angle;
}

void update_model(entity* ent) {
	ent->model.xz_orientation = ent->xz_orientation;
	update_head_orientation(ent);
}

static void get_entity_model_transformation(mat4 transform, entity* ent) {
	glm_mat4_identity(transform);
	glm_translate(transform, (vec3){0.5f, 0.5f, 0.5f});
	glm_rotate(transform, ent->model.xz_orientation, (vec3){0.0f, 1.0f, 0.0f});
	glm_translate(transform, (vec3){-0.5f, -0.5f, -0.5f});
}

void pack_entity_vertices(entity* ent, float** custom_model_data,
                          int* num_custom_verts) {
	if (ent == NULL || custom_model_data == NULL || num_custom_verts == NULL) {
		return;
	}

	int model_vert_count = 0;
	for (int element_idx = 0; element_idx < ent->model.element_count; element_idx++) {
		for (int face_idx = 0; face_idx < 6; face_idx++) {
			if (ent->model.elements[element_idx].has_face[face_idx]) {
				model_vert_count += 6;
			}
		}
	}

	if (model_vert_count == 0) {
		return;
	}

	int new_vert_count = (*num_custom_verts) + model_vert_count;
	float* tmp = realloc(*custom_model_data,
	                     new_vert_count * sizeof(float) * FLOATS_PER_MODEL_VERT);
	assert(tmp != NULL && "Failed to allocate memory for entity model data");
	*custom_model_data = tmp;

	mat4 transformation;
	get_entity_model_transformation(transformation, ent);

	int written_verts = 0;
	for (int element_idx = 0; element_idx < ent->model.element_count; element_idx++) {
		blockbench_vertex* vertices = NULL;
		unsigned int* indices = NULL;
		int vertex_count = 0;
		int index_count = 0;

		generate_element_vertices(&ent->model.elements[element_idx],
		                          ent->model.texture_size, &vertices, &indices,
		                          &vertex_count, &index_count);

		for (int i = 0; i < index_count; i++) {
			int dest_idx =
			    (*num_custom_verts + written_verts) * FLOATS_PER_MODEL_VERT;
			blockbench_vertex vert = vertices[indices[i]];

			vec4 pos = {vert.position[0], vert.position[1], vert.position[2], 1.0f};
			vec4 normal = {vert.normal[0], vert.normal[1], vert.normal[2], 0.0f};
			vec4 transformed_vert;
			vec4 transformed_normal;

			glm_mat4_mulv(transformation, pos, transformed_vert);
			glm_mat4_mulv(transformation, normal, transformed_normal);

			(*custom_model_data)[dest_idx + 0] = ent->x + transformed_vert[0];
			(*custom_model_data)[dest_idx + 1] = ent->y + transformed_vert[1];
			(*custom_model_data)[dest_idx + 2] = ent->z + transformed_vert[2];

			(*custom_model_data)[dest_idx + 3] = transformed_normal[0];
			(*custom_model_data)[dest_idx + 4] = transformed_normal[1];
			(*custom_model_data)[dest_idx + 5] = transformed_normal[2];

			(*custom_model_data)[dest_idx + 6] = vert.uv[0];
			(*custom_model_data)[dest_idx + 7] = vert.uv[1];

			written_verts++;
		}

		free(vertices);
		free(indices);
	}

	*num_custom_verts += written_verts;
}
