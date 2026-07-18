#include "player_animation.h"

#include "../../util/settings.h"

#include <string.h>
#include <assert.h>
#include <cglm/cglm.h>

void update_player_head(entity* ent) {
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

void update_player_limbs(entity* ent) {
	float t = 0.0f;

	if (ent->is_walking) {
		t = (float)(get_time(&ent->walk_animation_clock)) * ANIMATION_SCALER;
	} 

	float angle = sin(t * M_PI * 2.0f) * 30.0f; // blockbench uses degrees(but sin() uses radians =] )!

	for (int i = 0; i < ent->model.element_count; i++) {
		blockbench_element* target = &ent->model.elements[i];

		float element_angle = angle;
			
		if (strcmp(target->name, "r_arm") == 0 || strcmp(target->name, "l_leg") == 0) {
			element_angle *= -1.0f;		 
		}
		else if (strcmp(target->name, "l_arm") != 0 && strcmp(target->name, "r_leg") != 0) {
			continue;
		}

		// all limbs should rotate w.r.t. z axis 
		target->rotation_axis = 2;
		target->rotation_angle = element_angle;
	}
}

void update_player_model(entity* ent) {
	assert(ent != NULL && "ERROR: Cannot pass NULL entity in update_player_model().\n");
	assert(ent->type == PLAYER && "ERROR: Must pass PLAYER entity in update_player_model().\n");

	update_player_head(ent);
	update_player_limbs(ent);
}
