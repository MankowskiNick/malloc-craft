#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define PI 3.141592653

#define VBO_WIDTH 11

#define RADS(deg) ((deg) * (PI / 180.0f))

#define WORLD_POS_TO_CHUNK_POS(x) x >= 0 ? (int)(x / CHUNK_SIZE) : (int)(x / CHUNK_SIZE) - 1
#define F_WORLD_POS_TO_CHUNK_POS(x) x >= 0 ? (x / (float)CHUNK_SIZE) : (x / (float)CHUNK_SIZE) - 1.0f

typedef unsigned int uint;

static inline char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the contents (+1 for null terminator)
    char* buffer = malloc(length + 1);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // Read file contents
    size_t bytesRead = fread(buffer, 1, length, file);
    buffer[bytesRead] = '\0'; // Null-terminate the string

    fclose(file);
    return buffer;
}

#define TRUE 1
#define FALSE 0

#endif