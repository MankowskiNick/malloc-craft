#include "block.h"
#include <mesh.h>
#include "world.h"
#include <util.h>
#include <player/core/player.h>
#include "../physics/water.h"
#include <cglm/cglm.h>
#include <glad/glad.h>
#include <cerialize/cerialize.h>
#include <blockbench_loader.h>

block_type* TYPES;
int BLOCK_COUNT = 0;

typedef struct block_ray_result {
    float distance;
    short side;
    short rot;
    short water_level;
} block_ray_result;

void cache_model(char* model) {
    blockbench_model* m = get_blockbench_model(model);
    if (!m) {
        fprintf(stderr, "Failed to load Blockbench model: %s\n", model);
    }
}

void copy_orientation_model(block_type* block, int orientation, json_object obj) {
    if (orientation < 0 || orientation >= 6) {
        fprintf(stderr, "Invalid orientation index %d for block type %s\n", orientation, block->name);
        return;
    }

    if (obj.type == JSON_NULL) {
        return;
    }

    if (block->models[orientation]) {
        free(block->models[orientation]);
    }
    block->models[orientation] = strdup(obj.value.string);
    cache_model(block->models[orientation]);
}

void parse_model_orientations(block_type* block, json_object models_obj) {

    if (models_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Block type %s has invalid models object\n", block->name);
        return;
    }

    json_object north_obj = json_get_property(models_obj, "north");
    json_object south_obj = json_get_property(models_obj, "south");
    json_object east_obj = json_get_property(models_obj, "east");
    json_object west_obj = json_get_property(models_obj, "west");
    json_object up_obj = json_get_property(models_obj, "up");
    json_object down_obj = json_get_property(models_obj, "down");

    if ((north_obj.type != JSON_STRING && north_obj.type != JSON_NULL)
            || (south_obj.type != JSON_STRING && south_obj.type != JSON_NULL)
            || (east_obj.type != JSON_STRING && east_obj.type != JSON_NULL)
            || (west_obj.type != JSON_STRING && west_obj.type != JSON_NULL)
            || (up_obj.type != JSON_STRING && up_obj.type != JSON_NULL)
            || (down_obj.type != JSON_STRING && down_obj.type != JSON_NULL)) {
        fprintf(stderr, "Block type %s has invalid model or face_atlas_coords\n", block->name);
        return;
    }

    // Default to the main model if any orientation is missing
    for (int i = 0; i < 6; i++) {
        if (!block->models[i]) {
            block->models[i] = strdup(block->model);
        }
    }

    copy_orientation_model(block, (int)NORTH, north_obj);
    copy_orientation_model(block, (int)SOUTH, south_obj);
    copy_orientation_model(block, (int)EAST, east_obj);
    copy_orientation_model(block, (int)WEST, west_obj);
    copy_orientation_model(block, (int)UP, up_obj);
    copy_orientation_model(block, (int)DOWN, down_obj);
}

void map_json_to_types(json block_types) {

    if (block_types.failure || block_types.root.type != JSON_LIST) {
        fprintf(stderr, "Failed to deserialize block types: %s\n", block_types.error_text);
        return;
    }

    json_list list = block_types.root.value.list;

    if (list.count != BLOCK_COUNT) {
        fprintf(stderr, "Block types count mismatch: expected %d, got %i\n", BLOCK_COUNT, list.count);
        return;
    }


    for (int i = 0; i < BLOCK_COUNT; i++) {
        json_object obj = list.items[i];
        if (obj.type != JSON_OBJECT) {
            fprintf(stderr, "Block type %d is not an object\n", i);
            continue;
        }
        
        json_object id_obj = json_get_property(obj, "id");
        json_object name_obj = json_get_property(obj, "name");
        json_object transparent_obj = json_get_property(obj, "transparent");
        json_object liquid_obj = json_get_property(obj, "liquid");
        json_object face_atlas_coords_obj = json_get_property(obj, "face_atlas_coords");
        json_object is_foliage_obj = json_get_property(obj, "is_foliage");
        json_object model_obj = json_get_property(obj, "model");
        json_object models_obj = json_get_property(obj, "models");
        json_object oriented_obj = json_get_property(obj, "oriented");

        if (id_obj.type != JSON_NUMBER 
            || name_obj.type != JSON_STRING 
            || transparent_obj.type != JSON_BOOL 
            || liquid_obj.type != JSON_BOOL 
            || face_atlas_coords_obj.type != JSON_LIST 
            || is_foliage_obj.type != JSON_BOOL 
            || (model_obj.type != JSON_STRING && model_obj.type != JSON_NULL)
            || (models_obj.type != JSON_OBJECT && models_obj.type != JSON_NULL)
            || oriented_obj.type != JSON_BOOL) {
            fprintf(stderr, "Block type %d has invalid properties\n", i);
            continue;
        }

        if (model_obj.type != JSON_STRING && face_atlas_coords_obj.value.list.count != 6) {
            fprintf(stderr, "Block type %d has invalid model or face_atlas_coords\n", i);
            continue;
        }

        block_type* block = &TYPES[i];
        block->id = (uint)id_obj.value.number;
        block->name = strdup(name_obj.value.string);
        block->transparent = transparent_obj.value.boolean;
        block->liquid = liquid_obj.value.boolean;
        block->is_foliage = is_foliage_obj.value.boolean;
        block->model = model_obj.type == JSON_STRING ? strdup(model_obj.value.string) : NULL;
        block->is_custom_model = block->model != NULL ? 1 : 0;
        block->oriented = oriented_obj.value.boolean ? 1 : 0;

        for (int j = 0; j < 6; j++) {
            block->models[j] = NULL;
        }
        
        
        if (block->model) {
            cache_model(block->model);
            if (models_obj.type == JSON_OBJECT) {
                parse_model_orientations(block, models_obj);
            }
        }
        else {
            for (int j = 0; j < 6; j++) {
                json_object face_coord_obj = face_atlas_coords_obj.value.list.items[j];
                if (face_coord_obj.type != JSON_LIST || face_coord_obj.value.list.count != 2) {
                    fprintf(stderr, "Block type %d has invalid face_atlas_coords\n", i);
                    continue;
                }
                TYPES[i].face_atlas_coords[j][0] = (uint)face_coord_obj.value.list.items[0].value.number;
                TYPES[i].face_atlas_coords[j][1] = (uint)face_coord_obj.value.list.items[1].value.number;
            }
        }
    }
}

void block_init() {
    
    // load block types from file
    char* block_types_json = read_file_to_string("res/blocks.json");
    if (block_types_json == NULL) {
        fprintf(stderr, "Failed to read block types from file\n");
        return;
    }

    // Deserialize the block types
    json block_types = deserialize_json(block_types_json, strlen(block_types_json));
    if (block_types.failure) {
        fprintf(stderr, "Failed to deserialize block types: %s\n", block_types.error_text);
        free(block_types_json);
        exit(EXIT_FAILURE);
    }

    if (block_types.root.type != JSON_LIST) {
        fprintf(stderr, "Block types JSON is not a list\n");
        free(block_types_json);
        exit(EXIT_FAILURE);
    }

    BLOCK_COUNT = block_types.root.value.list.count;

    // Initialize block types array 
    TYPES = malloc(sizeof(block_type) * BLOCK_COUNT);
    if (TYPES == NULL) {
        fprintf(stderr, "Failed to allocate memory for block types\n");
        return;
    }

    map_json_to_types(block_types);
    json_free(&block_types);
    free(block_types_json);
}

short calculate_hit_side(vec3 position, vec3 dir, float t, int chunk_x, int chunk_y, int chunk_z, chunk* c) {
    // determine which side was hit
    float x = position[0] + t * dir[0];
    float y = position[1] + t * dir[1];
    float z = position[2] + t * dir[2];
    
    float cx = (float)(chunk_x + (c->x * CHUNK_SIZE));
    float cy = (float)chunk_y;
    float cz = (float)(chunk_z + (c->z * CHUNK_SIZE));
    
    float x_diff = x - (float)(chunk_x + (c->x * CHUNK_SIZE));
    float y_diff = y - (float)chunk_y;
    float z_diff = z - (float)(chunk_z + (c->z * CHUNK_SIZE));
    
    if (fabs(x_diff) < 0.01f) {
        return (short)WEST;
    }
    else if (fabs(x_diff - 1.0f) < 0.01f) {
        return (short)EAST;
    }
    else if (fabs(y_diff) < 0.01f) {
        return (short)UP;
    }
    else if (fabs(y_diff - 1.0f) < 0.01f) {
        return (short)DOWN;
    }
    else if (fabs(z_diff) < 0.01f) {
        return (short)SOUTH;
    }
    else if (fabs(z_diff - 1.0f) < 0.01f) {
        return (short)NORTH;
    }
    return (short)UNKNOWN_SIDE;
};

short calculate_rot(float dx, float dz) {
    if (dz >= dx) {
        if (dx >= -dz) {
            return (short)EAST;
        }
        else {
            return (short)NORTH;
        }
    }
    else {
        if (dx >= -dz) {
            return (short)SOUTH;
        }
        else {
            return (short)WEST;
        }
    }
    return (short)UNKNOWN_SIDE;
}

void get_empty_dist(camera cam, block_ray_result* out_result) {
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;
    int chunk_x = 0;
    int chunk_y = (int)position[1];
    int chunk_z = 0;

    chunk* c = get_chunk_at(position[0], position[2], &chunk_x, &chunk_z);

    short hit = false;
    short hit_side = (short)UNKNOWN_SIDE;
    short rot = 0;
    short water_level = 0;

    while (chunk_y >= 0 && chunk_y < CHUNK_HEIGHT && t <= MAX_REACH && !hit) {
        short block_id = 0;
        short orientation = 0;
        get_block_info(c->blocks[chunk_x][chunk_y][chunk_z], &block_id, &orientation, &rot, &water_level);

        if (block_id != get_block_id("air") && block_id != get_block_id("water")) {
            hit = true;
            hit_side = calculate_hit_side(position, dir, t, chunk_x, chunk_y, chunk_z, c);
            rot = calculate_rot(-dir[2], -dir[0]);
        }

        t += 0.005f;
        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};
        c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);
        chunk_y = (uint)pos[1];
    }

    if (out_result == NULL) {
        out_result = malloc(sizeof(block_ray_result));
    }

    out_result->distance = t;
    out_result->side = hit_side;
    out_result->rot = rot;
    out_result->water_level = water_level;
}

void break_block(game_data* data) {
    camera cam = data->player->cam;
    block_ray_result result;
    get_empty_dist(cam, &result);
    float t = result.distance;
    short hit_side = result.side;
    short rot = result.rot;
    short water_level = result.water_level;
    t += RAY_STEP;

    if (t > MAX_REACH) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    int chunk_x = 0;
    int chunk_y = 0;
    int chunk_z = 0;
    chunk_y = (int)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    set_block_info(data, c, chunk_x, chunk_y, chunk_z, get_block_id("air"), (short)UNKNOWN_SIDE, 0, water_level);
    
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z, data->player->position[0], data->player->position[2]);
    queue_chunk_for_sorting(new_mesh);
}

short get_block_id(char* block_type) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (strcmp(TYPES[i].name, block_type) == 0) {
            return TYPES[i].id;
        }
    }

    printf("ERROR: Block type '%s' not found\n", block_type);
    return -1;
}

bool check_block_foliage(short id) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (TYPES[i].id == id) {
            return TYPES[i].is_foliage;
        }
    }

    printf("ERROR: Block type '%i' not found\n", id);
    return false;
}

short get_selected_block(player player) {
    char* block_id = player.hotbar[player.selected_block];
    return get_block_id(block_id);
}

void place_block(game_data* data) {
    camera cam = data->player->cam;
    float t = 0.0f;
    short hit_side = (short)UNKNOWN_SIDE;
    short rot = 0;
    block_ray_result result;
    get_empty_dist(cam, &result);
    t = result.distance;
    hit_side = result.side;
    rot = result.rot;
    t -= RAY_STEP;

    if (t >= MAX_REACH - RAY_STEP) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    int chunk_x = 0;
    int chunk_y = 0;
    int chunk_z = 0;
    chunk_y = (uint)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    short selected_block = get_selected_block(*data->player);
    block_type* type = get_block_type(selected_block);

    // Water blocks need water level 7 (source) to render and flow
    short water_level = (type && type->liquid) ? 7 : 0;

    set_block_info(data, c, chunk_x, chunk_y, chunk_z, selected_block, hit_side, rot, water_level);

    // update chunk and adjacent chunks
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z, data->player->position[0], data->player->position[2]);
    queue_chunk_for_sorting(new_mesh);
}

// TODO: refactor to make more safe
block_type* get_block_type(short id) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (TYPES[i].id == id) {
            return &TYPES[i];
        }
    }
    return NULL;
}

void send_cube_vbo(VAO vao, VBO vbo) {
    float faceVertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    bind_vao(vao);
    buffer_data(vbo, GL_STATIC_DRAW, faceVertices, 6 * 3 * sizeof(float));
    f_add_attrib(&vbo, 0, 3, 0, 3 * sizeof(float)); // position
    use_vbo(vbo);
}

void update_selected_block(player* p) {
    if (p == NULL) {
        return;
    }

    camera cam = p->cam;
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;
    int chunk_x = 0;
    int chunk_y = (int)position[1];
    int chunk_z = 0;

    chunk* c = get_chunk_at(position[0], position[2], &chunk_x, &chunk_z);

    short hit = false;
    short water_id = get_block_id("water");

    while (chunk_y >= 0 && chunk_y < CHUNK_HEIGHT && t <= MAX_REACH && !hit) {
        short block_id = 0;
        short orientation = 0;
        short rot = 0;
        short water_level = 0;
        get_block_info(c->blocks[chunk_x][chunk_y][chunk_z], &block_id, &orientation, &rot, &water_level);

        // Skip air and water blocks
        if (block_id != get_block_id("air") && block_id != water_id) {
            hit = true;
            p->selected_block_pos[0] = chunk_x + (c->x * CHUNK_SIZE);
            p->selected_block_pos[1] = chunk_y;
            p->selected_block_pos[2] = chunk_z + (c->z * CHUNK_SIZE);
            p->selected_block_id = block_id;
            p->has_selected_block = true;
            break;
        }

        t += 0.005f;
        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};
        c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);
        chunk_y = (uint)pos[1];
    }

    // If no block was hit, clear the selection
    if (!hit) {
        p->has_selected_block = false;
        p->selected_block_id = 0;
    }
}

