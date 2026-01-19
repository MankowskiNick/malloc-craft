#ifndef player_H
#define player_H

#include <player/core/camera.h>
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
    int fly_mode;           // 1 if in fly mode, 0 if normal physics
    bool is_crouching;      // true if player is crouching, false otherwise
    float camera_height_offset;  // Current camera height offset (for smooth crouch transitions)

    int selected_block;
    char** hotbar;
    int hotbar_size;

    int* selected_block_pos;  // [x, y, z] of the block player is looking at
    short selected_block_id;  // Block ID of the selected block
    bool has_selected_block;  // Whether player is looking at a valid block
} player;

player player_init(char* player_file);
void apply_physics(player* player, float delta_ms);

#endif
