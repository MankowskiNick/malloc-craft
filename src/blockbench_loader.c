#include <blockbench_loader.h>
#include <hashmap.h>
#include <cerialize/cerialize.h>
#include <asset.h>
#include <cglm/cglm.h>
#include <settings.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Blockbench uses pixels, but we need block units (1 block = 16 pixels in Minecraft standard)
#define PIXELS_PER_BLOCK 16.0f

// Hash function for string keys (file paths)
size_t string_hash(char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

// String equality function  
bool string_equals(char* a, char* b) {
    return strcmp(a, b) == 0;
}

// Define the hashmap type for our model cache
DEFINE_HASHMAP(model_cache, char*, blockbench_model*, string_hash, string_equals)

// Global model cache
static model_cache_hashmap model_cache_map;
static bool cache_initialized = false;

// Internal structures for parsing
typedef struct {
    float from[3];
    float to[3];
    float rotation_angle;
    int rotation_axis;  // 0=x, 1=y, 2=z
    float rotation_origin[3];
    float face_uvs[6][4];  // [face][u1,v1,u2,v2] - north,east,south,west,up,down
    bool has_face[6];
} blockbench_element;

typedef struct {
    int texture_size[2];
    blockbench_element* elements;
    int element_count;
} parsed_blockbench;

// Face indices for cube generation - creates two triangles per face
// Triangle winding: counter-clockwise for outward-facing normals
// Each face has 4 vertices (0,1,2,3) arranged in counter-clockwise order
// First triangle: (0,1,2), Second triangle: (0,2,3)
static const int face_indices[6] = {0, 1, 2, 0, 2, 3}; // Two triangles per face

// Face normals - pointing outward from the cube
static const float face_normals[6][3] = {
    {0, 0, -1}, // north (-Z)
    {1, 0, 0},  // east (+X)
    {0, 0, 1},  // south (+Z)  
    {-1, 0, 0}, // west (-X)
    {0, 1, 0},  // up (+Y)
    {0, -1, 0}  // down (-Y)
};

// Parse rotation axis from string
int parse_rotation_axis(const char* axis) {
    if (strcmp(axis, "x") == 0) return 0;
    if (strcmp(axis, "y") == 0) return 1; 
    if (strcmp(axis, "z") == 0) return 2;
    return 1; // default to Y
}

// Convert degrees to radians
float deg_to_rad(float degrees) {
    return degrees * M_PI / 180.0f;
}

// Apply rotation matrix to a point around an origin
void rotate_point(float point[3], float angle, int axis, float origin[3]) {
    // Translate to origin
    float translated[3] = {
        point[0] - origin[0],
        point[1] - origin[1], 
        point[2] - origin[2]
    };
    
    float rad = deg_to_rad(angle);
    float cos_a = cos(rad);
    float sin_a = sin(rad);
    
    float rotated[3];
    
    switch (axis) {
        case 0: // X axis
            rotated[0] = translated[0];
            rotated[1] = translated[1] * cos_a - translated[2] * sin_a;
            rotated[2] = translated[1] * sin_a + translated[2] * cos_a;
            break;
        case 1: // Y axis  
            rotated[0] = translated[0] * cos_a + translated[2] * sin_a;
            rotated[1] = translated[1];
            rotated[2] = -translated[0] * sin_a + translated[2] * cos_a;
            break;
        case 2: // Z axis
            rotated[0] = translated[0] * cos_a - translated[1] * sin_a;
            rotated[1] = translated[0] * sin_a + translated[1] * cos_a;
            rotated[2] = translated[2];
            break;
    }
    
    // Translate back
    point[0] = rotated[0] + origin[0];
    point[1] = rotated[1] + origin[1];
    point[2] = rotated[2] + origin[2];
}

// Parse face UV coordinates from JSON
void parse_face_uv(json_object face_obj, float uv[4]) {
    json_object uv_obj = json_get_property(face_obj, "uv");
    if (uv_obj.type == JSON_LIST && uv_obj.value.list.count >= 4) {
        for (int i = 0; i < 4; i++) {
            json_object coord = uv_obj.value.list.items[i];
            if (coord.type == JSON_NUMBER) {
                uv[i] = coord.value.number;
            }
        }
    }
}

// Validate that face texture reference is valid (points to atlas)
bool validate_face_texture(json_object face_obj, const char* atlas_texture_key) {
    json_object texture_obj = json_get_property(face_obj, "texture");
    if (texture_obj.type == JSON_STRING) {
        // Check if texture reference matches expected atlas key (e.g., "#3")
        return strcmp(texture_obj.value.string, atlas_texture_key) == 0;
    }
    return false;
}

// Parse Blockbench JSON into internal structure
parsed_blockbench* parse_blockbench_json(const char* json_string) {
    json parsed = deserialize_json(json_string, strlen(json_string));
    if (parsed.failure) {
        fprintf(stderr, "Failed to parse Blockbench JSON: %s\n", parsed.error_text);
        return NULL;
    }
    
    if (parsed.root.type != JSON_OBJECT) {
        fprintf(stderr, "Blockbench JSON root is not an object\n");
        json_free(&parsed);
        return NULL;
    }
    
    parsed_blockbench* result = malloc(sizeof(parsed_blockbench));
    if (!result) {
        json_free(&parsed);
        return NULL;
    }
    
    // Parse texture size
    json_object texture_size_obj = json_get_property(parsed.root, "texture_size");
    if (texture_size_obj.type == JSON_LIST && texture_size_obj.value.list.count >= 2) {
        result->texture_size[0] = (int)texture_size_obj.value.list.items[0].value.number;
        result->texture_size[1] = (int)texture_size_obj.value.list.items[1].value.number;
    } else {
        result->texture_size[0] = 16;
        result->texture_size[1] = 16;
    }
    
    // Parse textures and validate against atlas
    json_object textures_obj = json_get_property(parsed.root, "textures");
    
    if (textures_obj.type == JSON_OBJECT) {
        // Check if texture references match the atlas
        for (int i = 0; i < textures_obj.value.object.node_count; i++) {
            json_node* node = &textures_obj.value.object.nodes[i];
            if (node->value.type == JSON_STRING) {
                // Validate texture matches atlas - extract filename from atlas path
                const char* atlas_filename = strrchr(ATLAS_PATH, '/');
                if (atlas_filename) {
                    atlas_filename++; // Skip the '/'
                } else {
                    atlas_filename = ATLAS_PATH;
                }
                
                // Remove extension from atlas filename for comparison
                char atlas_name[256];
                strncpy(atlas_name, atlas_filename, sizeof(atlas_name) - 1);
                atlas_name[sizeof(atlas_name) - 1] = '\0';
                char* dot = strrchr(atlas_name, '.');
                if (dot) *dot = '\0';
                
                // Check if Blockbench texture matches atlas
                if (strcmp(node->value.value.string, atlas_name) != 0) {
                    fprintf(stderr, "ERROR: Blockbench model texture '%s' does not match atlas texture '%s'\n", 
                            node->value.value.string, atlas_name);
                    fprintf(stderr, "       Blockbench models must use the atlas texture only\n");
                    free(result);
                    json_free(&parsed);
                    return NULL;
                }
                break;
            }
        }
    }
    
    // Parse elements array
    json_object elements_obj = json_get_property(parsed.root, "elements");
    if (elements_obj.type != JSON_LIST) {
        fprintf(stderr, "No elements array found in Blockbench JSON\n");
        free(result);
        json_free(&parsed);
        return NULL;
    }
    
    result->element_count = elements_obj.value.list.count;
    result->elements = malloc(sizeof(blockbench_element) * result->element_count);
    
    for (int i = 0; i < result->element_count; i++) {
        json_object element = elements_obj.value.list.items[i];
        blockbench_element* elem = &result->elements[i];
        
        // Parse from/to coordinates
        json_object from_obj = json_get_property(element, "from");
        json_object to_obj = json_get_property(element, "to");
        
        if (from_obj.type == JSON_LIST && from_obj.value.list.count >= 3) {
            elem->from[0] = from_obj.value.list.items[0].value.number / PIXELS_PER_BLOCK;
            elem->from[1] = from_obj.value.list.items[1].value.number / PIXELS_PER_BLOCK;
            elem->from[2] = from_obj.value.list.items[2].value.number / PIXELS_PER_BLOCK;
        }
        
        if (to_obj.type == JSON_LIST && to_obj.value.list.count >= 3) {
            elem->to[0] = to_obj.value.list.items[0].value.number / PIXELS_PER_BLOCK;
            elem->to[1] = to_obj.value.list.items[1].value.number / PIXELS_PER_BLOCK;
            elem->to[2] = to_obj.value.list.items[2].value.number / PIXELS_PER_BLOCK;
        }
        
        // Parse rotation
        elem->rotation_angle = 0;
        elem->rotation_axis = 1;
        elem->rotation_origin[0] = elem->rotation_origin[1] = elem->rotation_origin[2] = 0;
        
        json_object rotation_obj = json_get_property(element, "rotation");
        if (rotation_obj.type == JSON_OBJECT) {
            json_object angle_obj = json_get_property(rotation_obj, "angle");
            json_object axis_obj = json_get_property(rotation_obj, "axis");
            json_object origin_obj = json_get_property(rotation_obj, "origin");
            
            if (angle_obj.type == JSON_NUMBER) {
                elem->rotation_angle = angle_obj.value.number;
            }
            
            if (axis_obj.type == JSON_STRING) {
                elem->rotation_axis = parse_rotation_axis(axis_obj.value.string);
            }
            
            if (origin_obj.type == JSON_LIST && origin_obj.value.list.count >= 3) {
                elem->rotation_origin[0] = origin_obj.value.list.items[0].value.number / PIXELS_PER_BLOCK;
                elem->rotation_origin[1] = origin_obj.value.list.items[1].value.number / PIXELS_PER_BLOCK;
                elem->rotation_origin[2] = origin_obj.value.list.items[2].value.number / PIXELS_PER_BLOCK;
            }
        }
        
        // Parse faces
        json_object faces_obj = json_get_property(element, "faces");
        const char* face_names[] = {"north", "east", "south", "west", "up", "down"};
        
        for (int f = 0; f < 6; f++) {
            elem->has_face[f] = false;
            json_object face_obj = json_get_property(faces_obj, face_names[f]);
            if (face_obj.type == JSON_OBJECT) {
                elem->has_face[f] = true;
                parse_face_uv(face_obj, elem->face_uvs[f]);
            }
        }
    }
    
    json_free(&parsed);
    return result;
}

// Generate vertices for a single element
void generate_element_vertices(blockbench_element* elem, int texture_size[2], 
                              blockbench_vertex** vertices, unsigned int** indices,
                              int* vertex_count, int* index_count) {
    
    // Create 8 corner vertices of the box
    float corners[8][3] = {
        {elem->from[0], elem->from[1], elem->from[2]}, // 0: min,min,min
        {elem->to[0],   elem->from[1], elem->from[2]}, // 1: max,min,min  
        {elem->to[0],   elem->to[1],   elem->from[2]}, // 2: max,max,min
        {elem->from[0], elem->to[1],   elem->from[2]}, // 3: min,max,min
        {elem->from[0], elem->from[1], elem->to[2]},   // 4: min,min,max
        {elem->to[0],   elem->from[1], elem->to[2]},   // 5: max,min,max
        {elem->to[0],   elem->to[1],   elem->to[2]},   // 6: max,max,max
        {elem->from[0], elem->to[1],   elem->to[2]}    // 7: min,max,max
    };
    
    // Apply rotation if needed
    if (elem->rotation_angle != 0) {
        for (int i = 0; i < 8; i++) {
            rotate_point(corners[i], elem->rotation_angle, elem->rotation_axis, elem->rotation_origin);
        }
    }
    
    // Count faces that will be generated
    int face_count = 0;
    for (int f = 0; f < 6; f++) {
        if (elem->has_face[f]) face_count++;
    }
    
    int verts_per_face = 4;
    int indices_per_face = 6;
    
    *vertex_count = face_count * verts_per_face;
    *index_count = face_count * indices_per_face;
    
    *vertices = malloc(sizeof(blockbench_vertex) * (*vertex_count));
    *indices = malloc(sizeof(unsigned int) * (*index_count));
    
    int vertex_offset = 0;
    int index_offset = 0;
    
    // Standard cube face definitions (which corners belong to each face)
    // Each face is defined with 4 vertices in counter-clockwise order when viewed from outside
    // Corner indices reference the 8 cube corners defined above:
    // 0: min,min,min  1: max,min,min  2: max,max,min  3: min,max,min
    // 4: min,min,max  5: max,min,max  6: max,max,max  7: min,max,max
    int face_corners[6][4] = {
        {0, 3, 2, 1}, // north (-Z): front face, counter-clockwise from outside
        {1, 2, 6, 5}, // east (+X): right face, counter-clockwise from outside
        {5, 6, 7, 4}, // south (+Z): back face, counter-clockwise from outside
        {4, 7, 3, 0}, // west (-X): left face, counter-clockwise from outside
        {3, 7, 6, 2}, // up (+Y): top face, counter-clockwise from outside
        {1, 5, 4, 0}  // down (-Y): bottom face, counter-clockwise from outside
    };
    
    for (int f = 0; f < 6; f++) {
        if (!elem->has_face[f]) continue;
        
        // Get UV coordinates for this face (normalized [0,1] based on model texture size)
        float u1 = elem->face_uvs[f][0] / PIXELS_PER_BLOCK;//f / texture_size[0];
        float v1 = 1.0f - elem->face_uvs[f][1] / PIXELS_PER_BLOCK;// / texture_size[1];
        float u2 = elem->face_uvs[f][2] / PIXELS_PER_BLOCK;// / texture_size[0];
        float v2 = 1.0f - elem->face_uvs[f][3] / PIXELS_PER_BLOCK;// / texture_size[1];

        // Create 4 vertices for this face
        for (int v = 0; v < 4; v++) {
            int corner_idx = face_corners[f][v];
            blockbench_vertex* vert = &(*vertices)[vertex_offset + v];
            
            // Position from corner
            vert->position[0] = corners[corner_idx][0];
            vert->position[1] = corners[corner_idx][1]; 
            vert->position[2] = corners[corner_idx][2];
            
            // Face normal
            vert->normal[0] = face_normals[f][0];
            vert->normal[1] = face_normals[f][1];
            vert->normal[2] = face_normals[f][2];
            
            // UV mapping: vertex 0->u1,v1  vertex 1->u2,v1  vertex 2->u2,v2  vertex 3->u1,v2
            float uv_coords[4][2] = {{u1,v1}, {u2,v1}, {u2,v2}, {u1,v2}};
            vert->uv[0] = uv_coords[v][0];
            vert->uv[1] = uv_coords[v][1];
        }
        
        // Create 2 triangles for this face: (0,1,2) and (0,2,3)
        for (int i = 0; i < 6; i++) {
            (*indices)[index_offset + i] = vertex_offset + face_indices[i];
        }
        
        vertex_offset += 4;
        index_offset += 6;
    }
}

// Load and process Blockbench model from file
blockbench_model* load_blockbench_model_from_file(const char* filepath) {
    // Construct full path relative to res/ folder
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "res/%s", filepath);
    
    char* json_string = read_file_to_string(full_path);
    if (!json_string) {
        fprintf(stderr, "Failed to read Blockbench file: %s\n", full_path);
        return NULL;
    }
    
    parsed_blockbench* parsed = parse_blockbench_json(json_string);
    free(json_string);
    
    if (!parsed) {
        return NULL;
    }
    
    blockbench_model* model = malloc(sizeof(blockbench_model));
    if (!model) {
        free(parsed);
        return NULL;
    }
    
    // Copy basic info
    model->filepath = strdup(filepath);
    model->name = strdup(filepath); // Use filepath as name for now
    model->texture_size[0] = parsed->texture_size[0];
    model->texture_size[1] = parsed->texture_size[1];
    
    // Initialize bounds
    model->bounds_min[0] = model->bounds_min[1] = model->bounds_min[2] = INFINITY;
    model->bounds_max[0] = model->bounds_max[1] = model->bounds_max[2] = -INFINITY;
    
    // Generate vertices for all elements
    model->vertex_count = 0;
    model->index_count = 0;
    model->vertices = NULL;
    model->indices = NULL;
    
    for (int i = 0; i < parsed->element_count; i++) {
        blockbench_vertex* elem_vertices;
        unsigned int* elem_indices;
        int elem_vertex_count, elem_index_count;
        
        generate_element_vertices(&parsed->elements[i], parsed->texture_size,
                                &elem_vertices, &elem_indices, 
                                &elem_vertex_count, &elem_index_count);
        
        // Append to model arrays
        model->vertices = realloc(model->vertices, 
            sizeof(blockbench_vertex) * (model->vertex_count + elem_vertex_count));
        model->indices = realloc(model->indices,
            sizeof(unsigned int) * (model->index_count + elem_index_count));
        
        // Copy vertices and update bounds
        for (int v = 0; v < elem_vertex_count; v++) {
            model->vertices[model->vertex_count + v] = elem_vertices[v];
            
            // Update bounding box
            float* pos = elem_vertices[v].position;
            for (int axis = 0; axis < 3; axis++) {
                if (pos[axis] < model->bounds_min[axis]) model->bounds_min[axis] = pos[axis];
                if (pos[axis] > model->bounds_max[axis]) model->bounds_max[axis] = pos[axis];
            }
        }
        
        // Copy indices with offset
        for (int idx = 0; idx < elem_index_count; idx++) {
            model->indices[model->index_count + idx] = elem_indices[idx] + model->vertex_count;
        }
        
        model->vertex_count += elem_vertex_count;
        model->index_count += elem_index_count;
        
        free(elem_vertices);
        free(elem_indices);
    }
    
    // Cleanup parsed data
    free(parsed->elements);
    free(parsed);
    
    return model;
}

// Public API implementation
blockbench_model* get_blockbench_model(const char* filepath) {
    // Initialize cache on first use
    if (!cache_initialized) {
        model_cache_map = model_cache_init(64);
        cache_initialized = true;
    }
    
    // Check if already cached
    blockbench_model** cached = model_cache_get(&model_cache_map, (char*)filepath);
    if (cached) {
        return *cached;
    }
    
    // Load and process new model
    blockbench_model* model = load_blockbench_model_from_file(filepath);
    if (model) {
        // Store in cache
        char* path_copy = strdup(filepath);
        model_cache_insert(&model_cache_map, path_copy, model);
    }
    
    return model;
}

void blockbench_cleanup() {
    if (cache_initialized) {
        // TODO: Free all cached models and their data
        model_cache_free(&model_cache_map);
        cache_initialized = false;
    }
}
