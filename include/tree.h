#ifndef TREE_H
#define TREE_H
#include <chunk.h>

typedef struct { 
    short x, y, z;
} leaf_coord;

typedef struct {
    short id;
    char* name;
    short trunk_type;
    short leaf_type;
    short base_height;
    short height_variance;

    const short num_leaves;
    leaf_coord* leaf_pattern;
    float leaf_density;
} tree;

enum {
    OAK_TREE,
    CACTUS_TREE
};

void tree_init();
tree* get_tree_type(short id);

void generate_tree(int x, int y, int z, short id, chunk* c);

extern tree TREES[];

#endif