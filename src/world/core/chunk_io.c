#include "chunk_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_FILE_SIZE (CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE * sizeof(block_data_t))

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
    
    FILE* file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("Failed to open chunk file for writing");
        free(filepath);
        return -1;
    }
    
    // Write all blocks sequentially
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                size_t written = fwrite(&c->blocks[x][y][z], sizeof(block_data_t), 1, file);
                if (written != 1) {
                    perror("Failed to write block data");
                    fclose(file);
                    free(filepath);
                    return -1;
                }
            }
        }
    }
    
    fclose(file);
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
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                size_t read_count = fread(&c->blocks[x][y][z], sizeof(block_data_t), 1, file);
                if (read_count != 1) {
                    perror("Failed to read block data");
                    fclose(file);
                    free(filepath);
                    return -1;
                }
            }
        }
    }
    
    fclose(file);
    free(filepath);
    
    // Mark as not modified since we just loaded it
    c->modified = false;
    
    return 0;
}
