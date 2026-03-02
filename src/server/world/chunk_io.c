#include "chunk_io.h"
#include "../compression/compression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int init_worlds_directory(const char* worlds_dir) {
    struct stat st = {0};
    
    if (stat(worlds_dir, &st) == -1) {
        if (mkdir(worlds_dir, 0755) == -1) {
            perror("Failed to create worlds directory");
            return -1;
        }
    }
    
    return 0;
}

char* get_chunk_filepath(int x, int z, const char* worlds_dir) {
    size_t dir_len = strlen(worlds_dir);
    size_t needed = dir_len + 50;  // Extra space for path + filename
    char* filepath = malloc(needed);
    
    if (filepath == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    
    // Ensure worlds_dir ends with '/'
    if (worlds_dir[dir_len - 1] == '/') {
        snprintf(filepath, needed, "%schunk_%d_%d.bin", worlds_dir, x, z);
    } else {
        snprintf(filepath, needed, "%s/chunk_%d_%d.bin", worlds_dir, x, z);
    }
    
    return filepath;
}

int chunk_save_to_disk(chunk* c, const char* worlds_dir) {
    if (c == NULL || worlds_dir == NULL) {
        return -1;
    }
    
    char* filepath = get_chunk_filepath(c->x, c->z, worlds_dir);
    if (filepath == NULL) {
        return -1;
    }
    
    int compressed_size = 0;
    byte* compressed = compress_chunk(c, &compressed_size);
    if (compressed == NULL) {
        free(filepath);
        return -1;
    }

    FILE* file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("Failed to open chunk file for writing");
        free(compressed);
        free(filepath);
        return -1;
    }

    if (fwrite(&compressed_size, sizeof(int), 1, file) != 1) {
        perror("Failed to write compressed size");
        fclose(file);
        free(compressed);
        free(filepath);
        return -1;
    }

    if (fwrite(compressed, 1, compressed_size, file) != (size_t)compressed_size) {
        perror("Failed to write compressed chunk data");
        fclose(file);
        free(compressed);
        free(filepath);
        return -1;
    }

    fclose(file);
    free(compressed);
    free(filepath);
    return 0;
}

int chunk_load_from_disk(chunk* c, const char* worlds_dir) {
    if (c == NULL || worlds_dir == NULL) {
        return -1;
    }
    
    char* filepath = get_chunk_filepath(c->x, c->z, worlds_dir);
    if (filepath == NULL) {
        return -1;
    }
    
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        free(filepath);
        return -1;
    }

    int compressed_size = 0;
    if (fread(&compressed_size, sizeof(int), 1, file) != 1) {
        perror("Failed to read compressed size");
        fclose(file);
        free(filepath);
        return -1;
    }

    byte* compressed = malloc(compressed_size);
    if (compressed == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        free(filepath);
        return -1;
    }

    if (fread(compressed, 1, compressed_size, file) != (size_t)compressed_size) {
        perror("Failed to read compressed chunk data");
        fclose(file);
        free(compressed);
        free(filepath);
        return -1;
    }

    fclose(file);
    free(filepath);

    chunk* loaded = decompress_chunk(compressed, compressed_size);
    free(compressed);
    if (loaded == NULL) {
        return -1;
    }

    memcpy(c->blocks, loaded->blocks, sizeof(c->blocks));
    free(loaded);

    c->modified = false;
    return 0;
}
