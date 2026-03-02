#ifndef WORLD_STATE_H
#define WORLD_STATE_H

#include <pthread.h>

#include "../../world/core/chunk.h"

void init_world_state(void);
chunk* load_chunk_state(pthread_mutex_t* disk_lock, int x, int z);
void save_chunk_state(chunk* c);

#endif