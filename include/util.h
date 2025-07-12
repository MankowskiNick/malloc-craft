#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#define PI 3.141592653

#define RADS(deg) ((deg) * (PI / 180.0f))

#define CAMERA_POS_TO_CHUNK_POS(x) x >= 0 ? (int)(x / CHUNK_SIZE) : (int)(x / CHUNK_SIZE) - 1

typedef unsigned int uint;

#define TRUE 1
#define FALSE 0

#endif