#ifndef CHUNK_IO_H
#define CHUNK_IO_H

#include <block_models.h>

/**
 * Initialize the worlds directory. Creates {project_root}/worlds/ if it doesn't exist.
 * @param worlds_dir Path to the worlds directory
 * @return 0 on success, -1 on failure
 */
int init_worlds_directory(const char* worlds_dir);

/**
 * Save a chunk to disk in binary format.
 * Filename format: chunk_X_Z.bin
 * @param c Pointer to the chunk to save
 * @param worlds_dir Path to the worlds directory
 * @return 0 on success, -1 on failure
 */
int chunk_save_to_disk(chunk* c, const char* worlds_dir);

/**
 * Load a chunk from disk in binary format.
 * @param c Pointer to the chunk to load into (must have x, z already set)
 * @param worlds_dir Path to the worlds directory
 * @return 0 on success, -1 on failure (e.g., file doesn't exist)
 */
int chunk_load_from_disk(chunk* c, const char* worlds_dir);

/**
 * Get the filepath for a chunk file.
 * Caller is responsible for freeing the returned string.
 * @param x Chunk x coordinate
 * @param z Chunk z coordinate
 * @param worlds_dir Path to the worlds directory
 * @return Dynamically allocated string with the filepath, or NULL on failure
 */
char* get_chunk_filepath(int x, int z, const char* worlds_dir);

#endif
