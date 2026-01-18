#ifndef player_H
#define player_H

#include <camera.h>
#include <cerialize/cerialize.h>

typedef struct {
    camera cam;

    float* position;
    float* velocity;
    float* acceleration;    // Current acceleration [x, y, z] (forces applied each frame)
    
    float height;
    float radius;

    int is_grounded;        // 1 if touching ground, 0 if airborne
    int is_underwater;      // 1 if submerged in water, 0 if in air
    int coyote_counter;     // Frames since leaving ground (allows jumping within grace period)
    int jump_requested;     // 1 if jump input was triggered this frame

    int selected_block;
    char** hotbar;
    int hotbar_size;
} player;

player player_init(char* player_file);
void apply_physics(player* player, float delta_ms);

#endif