#include <sort.h>
#include <string.h>

void swap(void* a, void* b, size_t size) {
    char temp[size];
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

int partition(void* array, int low, int high, size_t elem_size, sort_key_fn get_key) {
    float pivot_key = get_key((char*)array + high * elem_size);
    
    int i = low - 1;
    
    for (int j = low; j < high; j++) {
        if (get_key((char*)array + j * elem_size) <= pivot_key) {
            i++;
            swap((char*)array + i * elem_size, (char*)array + j * elem_size, elem_size);
        }
    }
    
    // Swap the pivot element
    swap((char*)array + (i + 1) * elem_size, (char*)array + high * elem_size, elem_size);
    return i + 1;
}

// The main quicksort function
void _quicksort(void* array, int low, int high, size_t elem_size, sort_key_fn get_key) {
    if (low < high) {
        int pi = partition(array, low, high, elem_size, get_key);
        
        _quicksort(array, low, pi - 1, elem_size, get_key);
        _quicksort(array, pi + 1, high, elem_size, get_key);
    }
}

// Wrapper function to start the quicksort
void quicksort(void* array, size_t count, size_t elem_size, sort_key_fn get_key) {
    if (array == NULL || count <= 1)
        return;
        
    _quicksort(array, 0, count - 1, elem_size, get_key);
}