#ifndef SORT_H
#define SORT_H

#include <stdlib.h>

typedef float (*sort_key_fn)(const void* item);

void quicksort(void* array, size_t count, size_t elem_size, sort_key_fn get_key);

#endif