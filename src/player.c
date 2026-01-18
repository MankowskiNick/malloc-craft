#include <player.h>
#include <asset.h>
#include <world.h>
#include <block.h>
#include <mesh.h>

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
static short get_block_id_at(float x, float y, float z) {
    int chunk_x = 0;
    int chunk_z = 0;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    block_data_t data = get_block_data(chunk_x, (int)y, chunk_z, c);
    short id = 0;
    get_block_info(data, &id, NULL, NULL, NULL);

    return id;
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
        short block_id = get_block_id_at(bottom_checks[i][0], bottom_checks[i][1], bottom_checks[i][2]);
        if (block_id != air_id && block_id != water_id) {
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
        short block_id = get_block_id_at(top_checks[i][0], top_checks[i][1], top_checks[i][2]);
        if (block_id != air_id && block_id != water_id) {
            return 0;
        }
    }
    
    // Check center point at middle height
    short block_id = get_block_id_at(center_x, center_y + height * 0.5f, center_z);
    if (block_id != air_id && block_id != water_id) {
        return 0;
    }
    
    return 1;  // No collision
}

void update_player_pos(player* player, float direction[3]) {
    // load chunk and adjacent chunk data
    int chunk_x, chunk_z;
    chunk* current = get_chunk_at(player->position[0], player->position[2], &chunk_x, &chunk_z);
    
    chunk* adj[4] = {
        get_chunk(chunk_x, chunk_z - 1),
        get_chunk(chunk_x + 1, chunk_z),
        get_chunk(chunk_x, chunk_z + 1),
        get_chunk(chunk_x - 1, chunk_z)
    };

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
    // Gravity pulls downward (negative Y)
    float check_dist = delta_ms * GRAV_ACCEL / 1000.0f; 
    
    float direction[3] = {
        0.0f,
        check_dist,  // Already negative from GRAV_ACCEL
        0.0f
    };
    update_player_pos(player, direction);
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

    player player = {
        .cam = cam,

        .position = position,
        .height = height,
        .radius = radius,

        .selected_block = 0,
        .hotbar = hotbar,
        .hotbar_size = hotbar_obj.value.list.count,
    };

    player.cam.position[0] = player.position[0];
    player.cam.position[1] = player.position[1] + height;
    player.cam.position[2] = player.position[2];

    return player;
}
