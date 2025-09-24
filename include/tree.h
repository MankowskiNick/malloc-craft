#ifndef TREE_H
#define TREE_H
#include <chunk.h>

typedef struct {
    char* block;
    short num_leaves;
    short** coords;
} leaf_layout;

typedef struct {
    char* id;
    short trunk_type;
    short base_height;
    short height_variance;

    leaf_layout* leaf_pattern;
    short leaf_pattern_count;
    float leaf_density;
} tree;

void tree_init();
tree* get_tree_type(char* id);

void generate_tree(int x, int y, int z, char* id, chunk* c);

extern tree* TREES;

#endif