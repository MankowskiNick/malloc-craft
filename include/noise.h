#ifndef NOISE_H
#define NOISE_H

#include <util.h>

void n_init(uint seed);
float n_get(float x, float z, float freq, float amp, uint num_octaves);

#endif