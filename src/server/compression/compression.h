#ifndef COMPRESS_H
#define COMPRESS_H

#include <block_models.h>

// Data layout:
//  1. 4 bytes - x
//  2. 4 bytes - z
//  3. 4 bytes - packet_count
//  4. packet_count * 7 bytes - packet data
//      a. 3 bytes - block information(id, orientation, etc.)
//      b. 4 bytes - segment count
#define INT_BYTES 4
#define BLOCK_DATA_BYTES 3

typedef struct packet {
    int count;
    block_data_t data;
} packet;

typedef struct compressed_chunk {
    int x, z;
    int packet_count;
    packet* packets;
} compressed_chunk;

byte* compress_chunk(chunk* c, int* out_size);
chunk* decompress_chunk(byte* bytes, int size);

#endif