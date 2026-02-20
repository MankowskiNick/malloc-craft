#include "compression.h"

#include "../../world/core/chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <util.h>

void pack_int_to_buffer(byte* dest, int value) {
    if (dest == NULL) {
        printf("ERROR: Cannot pack to NULL chunk data buffer.\n");
        return;
    }

    int bits = 8 * INT_BYTES;
    for (int i = 1; i <= INT_BYTES; i++) {
        dest[i - 1] = (value >> (bits - i * 8)) & 0xFF;
    }
}

void pack_block_to_buffer(byte* dest, block_data_t value) {
    if (dest == NULL) {
        printf("ERROR: Cannot pack to NULL chunk data buffer.\n");
        return;
    }

    for (int i = 0; i < BLOCK_DATA_BYTES; i++) {
        dest[i] = value.bytes[i];
    }
}

// Encode compressed chunk to buffer
byte* to_buffer(compressed_chunk* c_comp, int* out_size) {
    if (c_comp == NULL) {
        return NULL;
    }

    int size = 3 * INT_BYTES + (BLOCK_DATA_BYTES + INT_BYTES) * c_comp->packet_count;
    byte* out = malloc(size);
    for (int i = 0; i < size; i++) {
        out[i] = 0;
    }

    pack_int_to_buffer(out, c_comp->x);
    pack_int_to_buffer(out + INT_BYTES, c_comp->z);
    pack_int_to_buffer(out + 2 * INT_BYTES, c_comp->packet_count);

    int buffer_idx = 3 * INT_BYTES;
    for (int i = 0; i < c_comp->packet_count; i++) {
        pack_block_to_buffer(out + buffer_idx, c_comp->packets[i].data);
        buffer_idx += BLOCK_DATA_BYTES;

        pack_int_to_buffer(out + buffer_idx, c_comp->packets[i].count);
        buffer_idx += INT_BYTES;
    }

    *out_size = size;
    return out;
}

int pull_int_from_buffer(byte* source) {
    if (source == NULL) {
        printf("ERROR: Cannot pull int from NULL chunk data buffer.\n");
        return 0;
    }

    int out = 0;
    for (int i = 0; i < INT_BYTES; i++) {
        out |= (*(source + i) << ((INT_BYTES - 1 - i) * 8));
    }
    return out;
}

block_data_t  pull_block_data_from_buffer(byte* source) {
    block_data_t out = {
        .bytes = {0, 0, 0}
    };

    if (source == NULL) {
        printf("ERROR: Cannot pull block data from NULL chunk data buffer.\n");
        return out;
    }

    out.bytes[0] = source[0];
    out.bytes[1] = source[1];
    out.bytes[2] = source[2];
    return out;
}

packet pull_packet_from_buffer(byte* source) {
    packet out = {
        .count = 0,
        .data = {
            .bytes = { 0, 0, 0 }
        }
    };

    if (source == NULL) {
        printf("ERROR: Cannot pull packet data from NULL chunk data buffer.\n");
        return out;
    }

    block_data_t data = pull_block_data_from_buffer(source);
    int count = pull_int_from_buffer(source + BLOCK_DATA_BYTES);

    out.data = data;
    out.count = count;
    return out;
}

// Extract compressed_chunk encoded in bytes
compressed_chunk* from_buffer(byte* source, int size) {
    if (source == NULL) {
        printf("ERROR: Cannot convert NULL to compressed chunk.\n");
        return NULL;
    }

    if (size <= 2 * INT_BYTES + INT_BYTES + BLOCK_DATA_BYTES + INT_BYTES) {
        printf("ERROR: Size of chunk buffer is smaller than the minimum number of bytes.\n");
        return NULL;
    }

    int x = pull_int_from_buffer(source);
    int z = pull_int_from_buffer(source + INT_BYTES);
    int packet_count = pull_int_from_buffer(source + 2 * INT_BYTES);


    int idx = 3 * INT_BYTES;
    packet* packets = malloc(packet_count * sizeof(packet));
    for (int i = 0; i < packet_count; i++) {
        packets[i] = pull_packet_from_buffer(source + idx);
        idx += BLOCK_DATA_BYTES + INT_BYTES;
    }

    compressed_chunk* out = malloc(sizeof(compressed_chunk));

    out->x = x;
    out->z = z;
    out->packet_count = packet_count;
    out->packets = packets;
    return out;
}

// Check equality for block_data_t
bool block_data_equals(block_data_t a, block_data_t b) {
    return a.bytes[0] == b.bytes[0]
        && a.bytes[1] == b.bytes[1]
        && a.bytes[2] == b.bytes[2];
}

// Flatten block data from chunk into array
void flatten_block_array(chunk* c, block_data_t out[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT]) {
    for (int k = 0; k < CHUNK_HEIGHT; k++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int i = 0; i < CHUNK_SIZE; i++) {
                int idx = i + j * CHUNK_SIZE + k * CHUNK_SIZE * CHUNK_SIZE;
                out[idx].bytes[0] = c->blocks[i][k][j].bytes[0];
                out[idx].bytes[1] = c->blocks[i][k][j].bytes[1];
                out[idx].bytes[2] = c->blocks[i][k][j].bytes[2];
            }
        }
    }
}

// Fill block data in chunk with data from array
void populate_chunk_blocks(chunk* c, block_data_t blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT]) {
    for (int k = 0; k < CHUNK_HEIGHT; k++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int i = 0; i < CHUNK_SIZE; i++) {
                int idx = i + j * CHUNK_SIZE + k * CHUNK_SIZE * CHUNK_SIZE;
                c->blocks[i][k][j].bytes[0] = blocks[idx].bytes[0];
                c->blocks[i][k][j].bytes[1] = blocks[idx].bytes[1];
                c->blocks[i][k][j].bytes[2] = blocks[idx].bytes[2];
            }
        }
    }
}


// A block "segment" is a maximal set of n sequentially arranged blocks.  
// For example, "aaaabbc" has 3 segments. 
//   1. Block 'a', size 4
//   2. Block 'b', size 2
//   3. Block 'c', size 1
packet compress_block_segment(block_data_t blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT], int* i) {
    block_data_t ref = blocks[*i];
    int size = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;

    int count = 0;

    while ((*i) < size && block_data_equals(blocks[*i], ref)) {
        count++;
        (*i)++;
    }

    packet p = {
        .data = ref,
        .count = count
    };
    return p;
}

byte* compress_chunk(chunk* c, int* out_size) {

    // flatten to array
    int block_count = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;
    block_data_t blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT];
    flatten_block_array(c, blocks);

    // first count how many packets we will have
    int idx = 0;
    int count = 0;
    while (idx < block_count) {
        block_data_t cur = blocks[idx];

        packet p = compress_block_segment(blocks, &idx);
        count++;
    }

    idx = 0;
    packet* packets = malloc(count * sizeof(packet));
    for (int i = 0; i < count; i++) {
        packet p = compress_block_segment(blocks, &idx);
        packets[i] = p;
    }

    compressed_chunk c_comp = {
        .x = c->x,
        .z = c->z,
        .packet_count = count,
        .packets = packets
    };
    
    byte* out = to_buffer(&c_comp, out_size);
    return out;
}

chunk* decompress_chunk(byte* bytes, int size) {
    compressed_chunk* c_comp = from_buffer(bytes, size);

    block_data_t blocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT] = {0};

    // populate block data array
    int idx = 0;
    for (int i = 0; i < c_comp->packet_count; i++) {
        packet p = c_comp->packets[i];
        for (int j = 0; j < p.count; j++) {
            blocks[idx].bytes[0] = p.data.bytes[0];
            blocks[idx].bytes[1] = p.data.bytes[1];
            blocks[idx].bytes[2] = p.data.bytes[2];
            idx++;
        }
    }

    chunk* c = malloc(sizeof(chunk));
    c->x = c_comp->x;
    c->z = c_comp->z;
    c->modified = false;
    populate_chunk_blocks(c, blocks);
    return c;
}