#ifndef BROADCAST_H
#define BROADCAST_H

#include <util.h>

void* run_broadcast_thread(void* args);
void add_to_broadcast_queue(byte* data, int size);

#endif