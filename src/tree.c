#include <tree.h>
#include <block.h>
#include <block_models.h>
#include <asset.h>
#include <cerialize/cerialize.h>

tree* TREES;
int TREE_COUNT = 0;

void map_json_to_leaf_pattern(json_object leaf_pattern, int tree_index) {
    if (leaf_pattern.type != JSON_LIST) {
        fprintf(stderr, "Leaf pattern is not a list\n");
        return;
    }

    json_list list = leaf_pattern.value.list;

    for (int i = 0; i < list.count; i++) {
        json_object obj = list.items[i];
        if (obj.type != JSON_OBJECT) {
            fprintf(stderr, "Leaf pattern %d is not an object\n", i);
            continue;
        }
        
        json_object block_obj = json_get_property(obj, "block");
        json_object coords_obj = json_get_property(obj, "coords");

        if (block_obj.type != JSON_STRING || coords_obj.type != JSON_LIST) {
            fprintf(stderr, "Leaf pattern %d has invalid properties\n", i);
            continue;
        }

        tree* t = &TREES[tree_index];
        TREES[tree_index].leaf_pattern[i].block = strdup(block_obj.value.string);
        TREES[tree_index].leaf_pattern[i].num_leaves = (short)coords_obj.value.list.count;
        TREES[tree_index].leaf_pattern[i].coords = malloc(sizeof(short*) * TREES[tree_index].leaf_pattern[i].num_leaves);

        for (int j = 0; j < TREES[tree_index].leaf_pattern[i].num_leaves; j++) {
            json_object coord_obj = coords_obj.value.list.items[j];
            if (coord_obj.type != JSON_LIST || coord_obj.value.list.count != 3) {
                fprintf(stderr, "Leaf pattern %d coord %d is invalid\n", i, j);
                continue;
            }
            TREES[tree_index].leaf_pattern[i].coords[j] = malloc(sizeof(short) * 3);
            TREES[tree_index].leaf_pattern[i].coords[j][0] = (short)coord_obj.value.list.items[0].value.number;
            TREES[tree_index].leaf_pattern[i].coords[j][1] = (short)coord_obj.value.list.items[1].value.number;
            TREES[tree_index].leaf_pattern[i].coords[j][2] = (short)coord_obj.value.list.items[2].value.number;
        }
    }
}

void map_json_to_trees(json tree_types) {

    if (tree_types.failure || tree_types.root.type != JSON_LIST) {
        fprintf(stderr, "Failed to deserialize tree types: %s\n", tree_types.error_text);
        return;
    }

    json_list list = tree_types.root.value.list;

    if (list.count != TREE_COUNT) {
        fprintf(stderr, "Tree types count mismatch: expected %d, got %i\n", TREE_COUNT, list.count);
        return;
    }

    for (int i = 0; i < TREE_COUNT; i++) {
        json_object obj = list.items[i];
        if (obj.type != JSON_OBJECT) {
            fprintf(stderr, "Tree type %d is not an object\n", i);
            continue;
        }
        
        json_object id_obj = json_get_property(obj, "id");
        json_object trunk_type_obj = json_get_property(obj, "trunk_type");
        json_object base_height_obj = json_get_property(obj, "base_height");
        json_object height_variance_obj = json_get_property(obj, "height_variance");
        json_object leaf_pattern_obj = json_get_property(obj, "leaf_pattern");
        json_object leaf_density_obj = json_get_property(obj, "leaf_density");

        if (id_obj.type != JSON_STRING || trunk_type_obj.type != JSON_STRING || 
            base_height_obj.type != JSON_NUMBER || height_variance_obj.type != JSON_NUMBER || 
            leaf_pattern_obj.type != JSON_LIST || leaf_density_obj.type != JSON_NUMBER) {
            fprintf(stderr, "Tree type %d has invalid properties\n", i);
            continue;
        }

        TREES[i].id = strdup(id_obj.value.string);
        TREES[i].trunk_type = strdup(trunk_type_obj.value.string);
        TREES[i].base_height = (short)base_height_obj.value.number;
        TREES[i].height_variance = (short)height_variance_obj.value.number;
        TREES[i].leaf_density = leaf_density_obj.value.number;
        TREES[i].leaf_pattern_count = (short)leaf_pattern_obj.value.list.count;
        TREES[i].leaf_pattern = malloc(sizeof(leaf_layout) * TREES[i].leaf_pattern_count);
        if (TREES[i].leaf_pattern == NULL) {
            fprintf(stderr, "Failed to allocate memory for leaf pattern %d\n", i);
            continue;
        }

        // leaf pattern
        map_json_to_leaf_pattern(leaf_pattern_obj, i);
    }
}

void tree_init() {
    // load tree types from json
    char* tree_types_json = read_file_to_string("res/trees.json");
    if (tree_types_json == NULL) {
        fprintf(stderr, "Failed to read tree types from file\n");
        return; 
    }

    // Deserialize JSON
    json tree_types = deserialize_json(tree_types_json, strlen(tree_types_json));
    if (tree_types.failure) {
        fprintf(stderr, "Failed to deserialize tree types JSON\n");
        free(tree_types_json);
        return;
    }

    if (tree_types.root.type != JSON_LIST) {
        fprintf(stderr, "Tree types JSON is not a list\n");
        free(tree_types_json);
        return;
    }

    TREE_COUNT = tree_types.root.value.list.count;
    TREES = malloc(sizeof(tree) * TREE_COUNT);

    if (TREES == NULL) {
        fprintf(stderr, "Failed to allocate memory for tree types\n");
        free(tree_types_json);
        return;
    }

    map_json_to_trees(tree_types);

    json_free(&tree_types);
    free(tree_types_json);
}

void tree_cleanup() {
    for (int i = 0; i < TREE_COUNT; i++) {
        tree* t = &TREES[i];
        if (t->leaf_pattern_count > 0) {
            for (int j = 0; j < t->leaf_pattern_count; j++) {
                free(t->leaf_pattern[j].block);
                for (int k = 0; k < t->leaf_pattern[j].num_leaves; k++) {
                    free(t->leaf_pattern[j].coords[k]);
                }
                free(t->leaf_pattern[j].coords);
            }
            free(t->leaf_pattern);
        }
    }
}

tree* get_tree_type(char* id) {
    for (int i = 0; i < TREE_COUNT; i++) {
        if (strcmp(TREES[i].id, id) == 0) {
            return &TREES[i];
        }
    }
    return NULL;
}


void generate_tree(int x, int y, int z, char* id, chunk* c) {
    tree* t = get_tree_type(id);
    int height = (int)(t->base_height + (rand() / (float)RAND_MAX) * t->height_variance);
    
    // trunk
    for (int i = 0; i < height; i++) {
        set_block_info(c, x, y + i, z, get_block_id(t->trunk_type), (short)UP, 0);
    }

    // leaves
    for (int i = 0; i < t->leaf_pattern_count; i++) {
        leaf_layout l = t->leaf_pattern[i];

        for (int j = 0; j < l.num_leaves; j++) {
            // don't show all leaves
            if (rand() / (float)RAND_MAX > t->leaf_density) {
                continue;
            }

            int lx = x + l.coords[j][0];
            int ly = y + height + l.coords[j][1];
            int lz = z + l.coords[j][2];
            short block_id;
            get_block_info(c->blocks[lx][ly][lz], &block_id, NULL, NULL);
            if (block_id == get_block_id("air")) {
                set_block_info(c, lx, ly, lz, get_block_id(l.block), (short)UP, 0);
            }
        }
    }
}