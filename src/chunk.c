#include <chunk.h>
#include <noise.h>
#include <biome.h>
#include <tree.h>
#include <block.h>
#include <mesh.h>
#include <world.h>
#include <water.h>
#include <stdlib.h>

uint chunk_hash(chunk_coord c) {
    uint hash = (unsigned int)((c.x * 73856093) ^ (c.z * 19349663)) + CHUNK_CACHE_SIZE;
    return hash % CHUNK_CACHE_SIZE;
}

int chunk_equals(chunk_coord a, chunk_coord b) {
    return a.x == b.x && a.z == b.z;
}

void c_init() {
    n_init(SEED);
    tree_init();
}

int get_block_height(chunk* c, float x, float z, biome* b) {
    float noise = (n_get(x, z, 
        WORLDGEN_BLOCKHEIGHT_FREQUENCY,
        WORLDGEN_BLOCKHEIGHT_AMPLITUDE,
        WORLDGEN_BLOCKHEIGHT_OCTAVES) - 0.5f) * 2.0f;

    float y_ = WORLDGEN_BASE_TERRAIN_HEIGHT + noise * WORLDGEN_BLOCKHEIGHT_MODIFIER;
    
    return (int)y_;
}

void set_block_info(game_data* game_data, chunk* c, int x, int y, int z, short id, short orientation, short rot, short water_level) {
    int data = 0;

    // first 10 bits are block id
    data = id & 0x3FF;
    // next 3 bits are orientation
    data |= (orientation & 0x7) << 10;

    // next 2 bits are rotation
    data |= (rot & 0x3) << 13;

    // last 3 bits are water level
    water_level = water_level < 0 ? 0 : water_level;
    water_level = water_level > 7 ? 7 : water_level;
    data |= (water_level & 0x7) << 15;

    c->blocks[x][y][z] = int_to_block_data(data);

    // Check for water flow AFTER block data is written
    if (game_data != NULL) {
        chunk* adj[4] = {
            get_chunk(c->x, c->z - 1),
            get_chunk(c->x + 1, c->z),
            get_chunk(c->x, c->z + 1),
            get_chunk(c->x - 1, c->z)
        };
        check_for_flow(game_data, c, adj, x, y, z);
    }
}

void get_block_info(block_data_t bd, short* id, short* orientation, short* rot, short* water_level) {
    int data = block_data_to_int(bd);
    if (id != NULL) {
        *id = data & 0x3FF;
    }
    if (orientation != NULL) {
        *orientation = (data >> 10) & 0x7;
    }
    if (rot != NULL) {
        *rot = (data >> 13) & 0x3;
    }
    if (water_level != NULL) {
        *water_level = (data >> 15) & 0x7;
    }
}

short calculate_water_level(int y) {
    if (y == WORLDGEN_WATER_LEVEL) {
        return 6; // Source water level
    }
    if (y < WORLDGEN_WATER_LEVEL) {
        return 7; // Source water level
    }
    return 0;
}

void generate_blocks(chunk* c, int x, int z) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;

            biome* b = get_biome(x_, z_);
            float y = get_block_height(c, x_, z_, b);
            
            for (int k = 0; k < CHUNK_HEIGHT; k++) {
                short water_level = calculate_water_level(k);

                if (k > y) {
                    if (k > WORLDGEN_WATER_LEVEL) {
                        set_block_info(NULL, c, i, k, j, get_block_id("air"), (short)UNKNOWN_SIDE, 0, 0);
                    }
                    else {
                        set_block_info(NULL, c, i, k, j, get_block_id("water"), (short)DOWN, 0, water_level);
                    }
                }
                else if (k == y) {
                    if (k < WORLDGEN_WATER_LEVEL) {
                        set_block_info(NULL, c, i, k, j, get_block_id(b->underwater_type), (short)DOWN, 0, 0);
                    }
                    else {
                        set_block_info(NULL, c, i, k, j, get_block_id(b->surface_type), (short)DOWN, 0, 0);
                    }
                }
                else if (k > y - 3) {
                    set_block_info(NULL, c, i, k, j, get_block_id(b->subsurface_type), (short)DOWN, 0, 0);
                }
                else {
                    set_block_info(NULL, c, i, k, j, get_block_id(b->underground_type), (short)DOWN, 0, 0);
                }
            }
        }
    }

    // generate foliage
    for (int i = 2; i < CHUNK_SIZE - 2; i++) {
        for (int j = 2; j < CHUNK_SIZE - 2; j++) {
            float x_ = (float)x + (float)i / (float)CHUNK_SIZE;
            float z_ = (float)z + (float)j / (float)CHUNK_SIZE;

            biome* b = get_biome(x_, z_);
            int y = get_block_height(c, x_, z_, b);

            if (y <= WORLDGEN_WATER_LEVEL) {
                continue;
            }

            int placed = 0;
            for (int f = 0; f < b->foliage_count; f++) {
                if (rand() / (float)RAND_MAX < b->foliage[f].density && !placed) {
                    placed = 1;

                    generate_tree(i, y + 1, j, b->foliage[f].type, c);
                }
            }
        }
    }
}

void chunk_create(chunk* c, int x, int z) {
    if (c == NULL) {
        c = malloc(sizeof(chunk));
    }

    c->x = x;
    c->z = z;

    generate_blocks(c, x, z);
}