#include <player.h>
#include <asset.h>
#include <world.h>
#include <block.h>
#include <mesh.h>
#include <settings.h>
#include <math.h>

camera parse_camera(json_object cam_obj) {
    camera cam;

    json_object position = json_get_property(cam_obj, "position");
    json_object up = json_get_property(cam_obj, "up");
    json_object front = json_get_property(cam_obj, "front");
    json_object yaw = json_get_property(cam_obj, "yaw");
    json_object pitch = json_get_property(cam_obj, "pitch");

    assert(up.type == JSON_LIST && up.value.list.count == 3 && "Player JSON \"up\" must be a list of 3 floats.");
    assert(front.type == JSON_LIST && front.value.list.count == 3 && "Player JSON \"front\" must be a list of 3 floats.");
    assert(yaw.type == JSON_NUMBER && "Player JSON \"yaw\" must be a number.");
    assert(pitch.type == JSON_NUMBER && "Player JSON \"pitch\" must be a number.");

    for (int i = 0; i < 3; i++) {

        assert(up.value.list.items[i].type == JSON_NUMBER
            && front.value.list.items[i].type == JSON_NUMBER
            && "Error: camera vector component is not a number in player.json\n");

        cam.up[i] = up.value.list.items[i].value.number;
        cam.front[i] = front.value.list.items[i].value.number;
    }

    cam.yaw = yaw.value.number;
    cam.pitch = pitch.value.number;

    return cam;
}

char** parse_hotbar(json_object hotbar_obj) {
    if (hotbar_obj.type != JSON_LIST) {
        fprintf(stderr, "Error: hotbar is not a list in player.json\n");
        exit(EXIT_FAILURE);
    }

    if (hotbar_obj.value.list.count > 10 || hotbar_obj.value.list.count < 1) {
        fprintf(stderr, "Error: hotbar must contain between 1 and 10 items in `player.json`\n");
        exit(EXIT_FAILURE);
    }

    char** hotbar = malloc(hotbar_obj.value.list.count * sizeof(char*));
    for (int i = 0; i < hotbar_obj.value.list.count; i++) {
        json_object item = hotbar_obj.value.list.items[i];
        if (item.type != JSON_STRING) {
            fprintf(stderr, "Error: hotbar item %d is not a string in player.json\n", i);
            exit(EXIT_FAILURE);
        }
        hotbar[i] = strdup(item.value.string);
    }

    return hotbar;
}

float parse_height(json_object height_obj) {
    assert(height_obj.type == JSON_NUMBER && "Player JSON \"height\" object must be a number.");
    return height_obj.value.number;
}

float parse_radius(json_object radius_obj) {
    assert(radius_obj.type == JSON_NUMBER && "Player JSON \"radius\" object must be a number.");
    return radius_obj.value.number;
}

float* parse_position(json_object position_object) {
    assert(position_object.type == JSON_LIST 
        && position_object.value.list.count == 3 
        && "Player JSON \"position\" must be a list of length 3.");

    for (int i = 0; i < 3; i++) {
        json_object obj = position_object.value.list.items[i];
        assert(obj.type == JSON_NUMBER && "Player JSON \"position\" must only contain floats/ints.");
    }

    float* pos = (float*)malloc(3 * sizeof(float));
    pos[0] = position_object.value.list.items[0].value.number;    
    pos[1] = position_object.value.list.items[1].value.number;
    pos[2] = position_object.value.list.items[2].value.number;

    return pos;
}

// Helper function to get block ID at world coordinates
static void get_block_info_at(float x, float y, float z, short* id, bool* is_foliage) {
    int chunk_x = 0;
    int chunk_z = 0;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    block_data_t data = get_block_data(chunk_x, (int)y, chunk_z, c);
    get_block_info(data, id, NULL, NULL, NULL);

    *is_foliage = check_block_foliage(*id);
}

// Check if a bounding box collides with solid blocks
static int check_collision_box(float center_x, float center_y, float center_z, float radius, float height, chunk* current, chunk* adj[4], int current_chunk_x, int current_chunk_z) {
    short air_id = get_block_id("air");
    short water_id = get_block_id("water");
    
    // Check 4 points at the bottom (feet level)
    float bottom_checks[4][3] = {
        {center_x - radius, center_y, center_z - radius},
        {center_x + radius, center_y, center_z - radius},
        {center_x - radius, center_y, center_z + radius},
        {center_x + radius, center_y, center_z + radius},
    };
    
    for (int i = 0; i < 4; i++) {
        short block_id = 0;
        bool is_foliage = false;
        get_block_info_at(bottom_checks[i][0], bottom_checks[i][1], bottom_checks[i][2], &block_id, &is_foliage);
        if (block_id != air_id && block_id != water_id && !is_foliage) {
            return 0;
        }
    }
    
    // Check 4 points at the top (head level)
    float top_checks[4][3] = {
        {center_x - radius, center_y + height, center_z - radius},
        {center_x + radius, center_y + height, center_z - radius},
        {center_x - radius, center_y + height, center_z + radius},
        {center_x + radius, center_y + height, center_z + radius},
    };
    
    for (int i = 0; i < 4; i++) {
        short block_id = 0;
        bool is_foliage = false;
        get_block_info_at(top_checks[i][0], top_checks[i][1], top_checks[i][2], &block_id, &is_foliage);
        if (block_id != air_id && block_id != water_id) {
            return 0;
        }
    }
    
    // Check center point at middle height
    short block_id = 0;
    bool is_foliage = false;
    get_block_info_at(center_x, center_y + height * 0.5f, center_z, &block_id, &is_foliage);
    if (block_id != air_id && block_id != water_id && !is_foliage) {
        return 0;
    }
    
    return 1;  // No collision
}

// Check if player is standing on solid ground
static int is_player_grounded(player* p, chunk* current, chunk* adj[4], int current_chunk_x, int current_chunk_z) {
    short air_id = get_block_id("air");
    short water_id = get_block_id("water");
    
    // Check 4 points slightly below feet level
    float check_dist = 0.05f;
    float ground_checks[4][3] = {
        {p->position[0] - p->radius, p->position[1] - check_dist, p->position[2] - p->radius},
        {p->position[0] + p->radius, p->position[1] - check_dist, p->position[2] - p->radius},
        {p->position[0] - p->radius, p->position[1] - check_dist, p->position[2] + p->radius},
        {p->position[0] + p->radius, p->position[1] - check_dist, p->position[2] + p->radius},
    };
    
    for (int i = 0; i < 4; i++) {
        short block_id = 0;
        bool is_foliage = false;
        get_block_info_at(ground_checks[i][0], ground_checks[i][1], ground_checks[i][2], &block_id, &is_foliage);
        // Grounded if touching solid block (not air, not water, not foliage)
        if (block_id != air_id && block_id != water_id && !is_foliage) {
            return 1;
        }
    }
    
    return 0;  // Not grounded
}

// Check if player is submerged in water
static int is_player_underwater(player* p, chunk* current, chunk* adj[4], int current_chunk_x, int current_chunk_z) {
    short air_id = get_block_id("air");
    short water_id = get_block_id("water");
    
    // Check if player's center/head is in water
    float center_x = p->position[0];
    float center_y = p->position[1] + p->height * 0.5f;  // Check at middle of body
    float center_z = p->position[2];
    
    short block_id = 0;
    bool is_foliage = false;
    get_block_info_at(center_x, center_y, center_z, &block_id, &is_foliage);
    
    // Underwater if we're in a water block
    return (block_id == water_id);
}

void update_player_pos(player* player, float delta_ms) {
    // load chunk and adjacent chunk data
    int chunk_x, chunk_z;
    chunk* current = get_chunk_at(player->position[0], player->position[2], &chunk_x, &chunk_z);
    
    chunk* adj[4] = {
        get_chunk(chunk_x, chunk_z - 1),
        get_chunk(chunk_x + 1, chunk_z),
        get_chunk(chunk_x, chunk_z + 1),
        get_chunk(chunk_x - 1, chunk_z)
    };

    float* direction = player->velocity;
    direction[0] *= delta_ms / 1000.0f;
    direction[1] *= delta_ms / 1000.0f;
    direction[2] *= delta_ms / 1000.0f;

    // Try X movement
    if (check_collision_box(player->position[0] + direction[0], player->position[1], player->position[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[0] += direction[0];
        player->cam.position[0] += direction[0];
    }

    // Try Z movement
    if (check_collision_box(player->position[0], player->position[1], player->position[2] + direction[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[2] += direction[2];
        player->cam.position[2] += direction[2];
    }

    // Try Y movement
    if (check_collision_box(player->position[0], player->position[1] + direction[1], player->position[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[1] += direction[1];
        player->cam.position[1] += direction[1];
    }
}

void apply_physics(player* player, float delta_ms) {
    // In fly mode, handle movement differently
    if (player->fly_mode) {
        float dt = delta_ms / 1000.0f;  // Convert ms to seconds
        
        // In fly mode: no gravity, no collision checking, but apply friction to stop movement
        // Normalize and apply 3D acceleration
        float accel_mag = sqrtf(player->acceleration[0] * player->acceleration[0] + 
                                player->acceleration[1] * player->acceleration[1] +
                                player->acceleration[2] * player->acceleration[2]);
        if (accel_mag > 0.0f) {
            // Normalize direction
            float dir_x = player->acceleration[0] / accel_mag;
            float dir_y = player->acceleration[1] / accel_mag;
            float dir_z = player->acceleration[2] / accel_mag;
            
            // Apply acceleration in normalized direction
            player->velocity[0] += dir_x * PLAYER_ACCEL * dt;
            player->velocity[1] += dir_y * PLAYER_ACCEL * dt;
            player->velocity[2] += dir_z * PLAYER_ACCEL * dt;
        }
        
        // Apply friction to velocity (for deceleration when no keys pressed)
        player->velocity[0] *= PLAYER_FRICTION;
        player->velocity[1] *= PLAYER_FRICTION;
        player->velocity[2] *= PLAYER_FRICTION;
        
        // Clamp max speed
        float total_speed = sqrtf(player->velocity[0] * player->velocity[0] + 
                                  player->velocity[1] * player->velocity[1] +
                                  player->velocity[2] * player->velocity[2]);
        if (total_speed > PLAYER_MAX_SPEED) {
            float speed_factor = PLAYER_MAX_SPEED / total_speed;
            player->velocity[0] *= speed_factor;
            player->velocity[1] *= speed_factor;
            player->velocity[2] *= speed_factor;
        }
        
        // Apply velocity to position without collision checking
        float direction[3] = {
            player->velocity[0] * dt,
            player->velocity[1] * dt,
            player->velocity[2] * dt
        };
        
        player->position[0] += direction[0];
        player->position[1] += direction[1];
        player->position[2] += direction[2];
        
        player->cam.position[0] += direction[0];
        player->cam.position[1] += direction[1];
        player->cam.position[2] += direction[2];
        
        return;
    }

    // Load chunk and adjacent chunk data
    int chunk_x, chunk_z;
    chunk* current = get_chunk_at(player->position[0], player->position[2], &chunk_x, &chunk_z);
    
    chunk* adj[4] = {
        get_chunk(chunk_x, chunk_z - 1),
        get_chunk(chunk_x + 1, chunk_z),
        get_chunk(chunk_x, chunk_z + 1),
        get_chunk(chunk_x - 1, chunk_z)
    };

    float dt = delta_ms / 1000.0f;  // Convert ms to seconds

    // 1. Underwater Detection
    int was_underwater = player->is_underwater;
    player->is_underwater = is_player_underwater(player, current, adj, chunk_x, chunk_z);

    // 2. Ground Detection
    int was_grounded = player->is_grounded;
    player->is_grounded = is_player_grounded(player, current, adj, chunk_x, chunk_z);
    
    // Update coyote counter (grace period for jumping after leaving ground)
    if (player->is_grounded) {
        player->coyote_counter = 0;
    } else if (was_grounded) {
        // Just left ground, start coyote timer
        player->coyote_counter = 1;
    } else if (player->coyote_counter > 0 && player->coyote_counter < COYOTE_TIME) {
        player->coyote_counter++;
    }

    // 3. Jump Input (disabled when underwater)
    if (player->jump_requested) {
        if (!player->is_underwater && (player->is_grounded || player->coyote_counter < COYOTE_TIME)) {
            // Apply jump impulse (only on ground, not underwater)
            // Apply water jump boost if jumping from grounded water position
            float jump_velocity = JUMP_FORCE;
            if (was_underwater) {
                jump_velocity *= WATER_JUMP_BOOST;
            }
            player->velocity[1] = jump_velocity;
            player->coyote_counter = COYOTE_TIME + 1;  // Consume coyote time
        }
        player->jump_requested = 0;  // Always consume jump input
    }

    // 4. Select physics parameters based on underwater state
    float accel = player->is_underwater ? SWIM_ACCEL : PLAYER_ACCEL;
    float friction = player->is_underwater ? WATER_FRICTION : PLAYER_FRICTION;
    float max_speed = player->is_underwater ? WATER_MAX_SPEED : PLAYER_MAX_SPEED;
    float gravity = player->is_underwater ? (GRAV_ACCEL * WATER_DRAG) : GRAV_ACCEL;

    // 5. Normalize and apply 3D acceleration (vertical included when underwater via input)
    float accel_mag = sqrtf(player->acceleration[0] * player->acceleration[0] + 
                            player->acceleration[1] * player->acceleration[1] +
                            player->acceleration[2] * player->acceleration[2]);
    if (accel_mag > 0.0f) {
        // Normalize direction
        float dir_x = player->acceleration[0] / accel_mag;
        float dir_y = player->acceleration[1] / accel_mag;
        float dir_z = player->acceleration[2] / accel_mag;
        
        // Apply acceleration in normalized direction
        float vertical_accel = accel;
        // Apply water jump boost to upward movement when transitioning out of water
        if (was_underwater && !player->is_underwater && dir_y > 0.0f) {
            vertical_accel *= WATER_JUMP_BOOST;
        }
        
        player->velocity[0] += dir_x * accel * dt;
        player->velocity[1] += dir_y * vertical_accel * dt;  // Vertical acceleration when swimming
        player->velocity[2] += dir_z * accel * dt;
    }

    // 6. Apply gravity (always applied, but reduced underwater by WATER_DRAG)
    player->velocity[1] += gravity * dt;

    // 7. Apply friction (only horizontal when not underwater, full 3D when underwater)
    if (player->is_underwater) {
        player->velocity[0] *= friction;
        player->velocity[1] *= friction;
        player->velocity[2] *= friction;
    } else {
        player->velocity[0] *= friction;
        player->velocity[2] *= friction;
    }

    // 8. Clamp max speed (horizontal for ground, all directions for underwater)
    if (player->is_underwater) {
        float total_speed = sqrtf(player->velocity[0] * player->velocity[0] + 
                                  player->velocity[1] * player->velocity[1] +
                                  player->velocity[2] * player->velocity[2]);
        if (total_speed > max_speed) {
            float speed_factor = max_speed / total_speed;
            player->velocity[0] *= speed_factor;
            player->velocity[1] *= speed_factor;
            player->velocity[2] *= speed_factor;
        }
    } else {
        float horizontal_speed = sqrtf(player->velocity[0] * player->velocity[0] + 
                                       player->velocity[2] * player->velocity[2]);
        if (horizontal_speed > max_speed) {
            float speed_factor = max_speed / horizontal_speed;
            player->velocity[0] *= speed_factor;
            player->velocity[2] *= speed_factor;
        }
    }

    // 9. Apply velocity to position
    float direction[3] = {
        player->velocity[0] * dt,
        player->velocity[1] * dt,
        player->velocity[2] * dt
    };

    // Try X movement
    if (check_collision_box(player->position[0] + direction[0], player->position[1], player->position[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[0] += direction[0];
        player->cam.position[0] += direction[0];
    } else {
        player->velocity[0] = 0.0f;  // Stop horizontal velocity on collision
    }

    // Try Z movement
    if (check_collision_box(player->position[0], player->position[1], player->position[2] + direction[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[2] += direction[2];
        player->cam.position[2] += direction[2];
    } else {
        player->velocity[2] = 0.0f;  // Stop horizontal velocity on collision
    }

    // Try Y movement
    if (check_collision_box(player->position[0], player->position[1] + direction[1], player->position[2], player->radius, player->height, current, adj, chunk_x, chunk_z)) {
        player->position[1] += direction[1];
        player->cam.position[1] += direction[1];
    } else {
        player->velocity[1] = 0.0f;  // Stop vertical velocity on collision
    }
}

player player_init(char* player_file) {

    char* player_json = read_file_to_string(player_file);
    json obj = deserialize_json(player_json, strlen(player_json));

    json_object hotbar_obj = json_get_property(obj.root, "hotbar");
    json_object cam_obj = json_get_property(obj.root, "cam");
    json_object height_obj = json_get_property(obj.root, "height");
    json_object radius_obj = json_get_property(obj.root, "radius");
    json_object position_object = json_get_property(obj.root, "position");

    char** hotbar = parse_hotbar(hotbar_obj);
    camera cam = parse_camera(cam_obj);
    float height = parse_height(height_obj);
    float radius = parse_radius(radius_obj);
    float* position = parse_position(position_object);

    float* velocity = malloc(3 * sizeof(float));
    velocity[0] = 0.0f;
    velocity[1] = 0.0f;
    velocity[2] = 0.0f;

    float* acceleration = malloc(3 * sizeof(float));
    acceleration[0] = 0.0f;
    acceleration[1] = 0.0f;
    acceleration[2] = 0.0f;

    int* selected_block_pos = malloc(3 * sizeof(int));
    selected_block_pos[0] = 0;
    selected_block_pos[1] = 0;
    selected_block_pos[2] = 0;

    player player = {
        .cam = cam,

        .position = position,
        .velocity = velocity,
        .acceleration = acceleration,

        .height = height,
        .radius = radius,

        .is_grounded = 0,
        .is_underwater = 0,
        .coyote_counter = 0,
        .jump_requested = 0,
        .fly_mode = 0,

        .selected_block = 0,
        .hotbar = hotbar,
        .hotbar_size = hotbar_obj.value.list.count,

        .selected_block_pos = selected_block_pos,
        .selected_block_id = 0,
        .has_selected_block = false,
    };

    player.cam.position[0] = player.position[0];
    player.cam.position[1] = player.position[1] + height;
    player.cam.position[2] = player.position[2];

    return player;
}
