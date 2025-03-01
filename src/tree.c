#include <tree.h>
#include <block_models.h>

void get_oak_leaf_pattern(leaf_coord* pattern, int num_leaves) {
    int offset = 0;

    // base
    for (int x = -2; x < 3; x++) {
        for (int y = -1; y < 2; y++) {
            for (int z = -2; z < 3; z++) {
                pattern[offset].x = x;
                pattern[offset].y = y;
                pattern[offset].z = z;
                offset++;
            }
        }
    }

    // top
    for (int x = -1; x < 2; x++) {
        for (int z = -1; z < 2; z++) {
            pattern[offset].x = x;
            pattern[offset].y = 2;
            pattern[offset].z = z;
            offset++;
        }
    }
}

void get_cactus_leaf_pattern(leaf_coord* pattern, int num_leaves) {
    pattern[0].x = 0;
    pattern[0].y = 0;
    pattern[0].z = 0;
}

void tree_init() {
    tree* oak = &TREES[OAK_TREE];
    oak->leaf_pattern = malloc(sizeof(leaf_coord) * oak->num_leaves);
    get_oak_leaf_pattern(oak->leaf_pattern, oak->num_leaves);

    tree* cactus = &TREES[CACTUS_TREE];
    cactus->leaf_pattern = malloc(sizeof(leaf_coord) * cactus->num_leaves);
    get_cactus_leaf_pattern(cactus->leaf_pattern, cactus->num_leaves);
}

void tree_cleanup() {
    for (int i = 0; i < TREE_COUNT; i++) {
        tree* t = &TREES[i];
        if (t->num_leaves > 0) {
            free(t->leaf_pattern);
        }
    }
}

tree* get_tree_type(short id) {
    for (int i = 0; i < TREE_COUNT; i++) {
        if (TREES[i].id == id) {
            return &TREES[i];
        }
    }
    return NULL;
}


void generate_tree(int x, int y, int z, short id, chunk* c) {
    tree* t = get_tree_type(id);
    int height = (int)(t->base_height + (rand() / (float)RAND_MAX) * t->height_variance);
    
    // trunk
    for (int i = 0; i < height; i++) {
        c->blocks[x][y + i][z] = t->trunk_type;
    }

    // leaves
    for (int i = 0; i < t->num_leaves; i++) {
        leaf_coord l = t->leaf_pattern[i];

        // don't show all leaves
        if (rand() / (float)RAND_MAX > t->leaf_density) {
            continue;
        }

        int lx = x + l.x;
        int ly = y + height + l.y;
        int lz = z + l.z;
        if (c->blocks[lx][ly][lz] == AIR) {
            c->blocks[x + l.x][y + height + l.y][z + l.z] = t->leaf_type;
        }
    }
}

tree TREES[] = {
    {
        .id = OAK_TREE,
        .name = "Oak Tree",
        .trunk_type = OAK_LOG,
        .leaf_type = OAK_LEAF,
        .base_height = 6,
        .height_variance = 5,
        .leaf_pattern = NULL,
        .num_leaves = 84,
        .leaf_density = 0.80f
    },
    {
        .id = CACTUS_TREE,
        .name = "Cactus",
        .trunk_type = CACTUS, // temporary, no cactus texture yet
        .leaf_type = CACTUS_TOP,
        .base_height = 3,
        .height_variance = 5,
        .leaf_pattern = NULL,
        .num_leaves = 1,
        .leaf_density = 1.0
    }
};