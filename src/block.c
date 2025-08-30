#include <block.h>
// #include <render.h>
#include <mesh.h>
#include <world.h>
#include <asset.h>
#include <player_instance.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include <cerialize/cerialize.h>

block_type* TYPES;

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

        if (id_obj.type != JSON_NUMBER && name_obj.type != JSON_STRING && 
            transparent_obj.type != JSON_BOOL && liquid_obj.type != JSON_BOOL && 
            face_atlas_coords_obj.type != JSON_LIST) {
            fprintf(stderr, "Block type %d has invalid properties\n", i);
            continue;
        }

        TYPES[i].id = (uint)id_obj.value.number;
        TYPES[i].name = strdup(name_obj.value.string);
        TYPES[i].transparent = transparent_obj.value.boolean;
        TYPES[i].liquid = liquid_obj.value.boolean;
        block_type* type = &TYPES[i];


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

void block_init() {
    
    // load block types from file
    char* block_types_json = read_file_to_string("res/blocks.json");
    if (block_types_json == NULL) {
        fprintf(stderr, "Failed to read block types from file\n");
        return;
    }

    // Deserialize the block types
    json block_types = deserialize_json(block_types_json, strlen(block_types_json));
    free(block_types_json);

    // Initialize block types array
    TYPES = malloc(sizeof(block_type) * BLOCK_COUNT);
    if (TYPES == NULL) {
        fprintf(stderr, "Failed to allocate memory for block types\n");
        return;
    }

    map_json_to_types(block_types);
}

float get_empty_dist(camera cam) {
    vec3 position = {cam.position[0], cam.position[1], cam.position[2]};
    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);

    float t = 0.0f;

    uint chunk_x, chunk_y, chunk_z;
    chunk_y = (uint)position[1];
    chunk* c = get_chunk_at(position[0], position[2], &chunk_x, &chunk_z);

    while (chunk_y >= 0 && chunk_y < CHUNK_HEIGHT 
        && t <= MAX_REACH && 
        c->blocks[chunk_x][chunk_y][chunk_z] == AIR || c->blocks[chunk_x][chunk_y][chunk_z] == WATER) {
        t += 0.005f;

        vec3 pos = {position[0] + dir[0] * t, position[1] + dir[1] * t, position[2] + dir[2] * t};

        c = get_chunk_at(pos[0], pos[2], &chunk_x, &chunk_z);
        chunk_y = (uint)pos[1];
    }

    return t;
}

void break_block(player_instance player) {
    camera cam = player.cam;
    float t = get_empty_dist(cam);
    t += RAY_STEP;

    if (t > MAX_REACH) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    uint chunk_x = 0;
    uint chunk_y = 0;
    uint chunk_z = 0;
    chunk_y = (uint)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    c->blocks[chunk_x][chunk_y][chunk_z] = AIR;
    
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z);
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

short get_selected_block(player_instance player) {
    char* block_id = player.hotbar[player.selected_block];
    return get_block_id(block_id);
}

void place_block(player_instance player) {
    camera cam = player.cam;
    float t = get_empty_dist(cam);
    t -= RAY_STEP;

    if (t >= MAX_REACH - RAY_STEP) {
        return;
    }

    vec3 dir = {cam.front[0], cam.front[1], cam.front[2]};
    glm_normalize_to(dir, dir);
    float x = cam.position[0] + t * dir[0];
    float y = cam.position[1] + t * dir[1];
    float z = cam.position[2] + t * dir[2];

    uint chunk_x = 0;
    uint chunk_y = 0;
    uint chunk_z = 0;
    chunk_y = (uint)y;
    chunk* c = get_chunk_at(x, z, &chunk_x, &chunk_z);

    if (chunk_x == -1 || chunk_y == -1 || chunk_z == -1 || c == NULL || chunk_y > CHUNK_HEIGHT - 1 || chunk_y < 0) {
        return;
    }

    c->blocks[chunk_x][chunk_y][chunk_z] = get_selected_block(player);

    // update chunk and adjacent chunks
    chunk_mesh* new_mesh = update_chunk_mesh(c->x, c->z);
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
