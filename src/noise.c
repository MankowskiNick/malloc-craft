#include "noise.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "settings.h"

int permutation[256];

float smooth(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
} 

float lerp(float t, float a, float b) {
    return t * (b - a) + a;
}

float grad(int hash, float x, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : z;
    float v = h < 4 ? z : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin(float x, float z) {
    // Find unit square that contains point
    int X = (int)floor(x) & 255;
    int Z = (int)floor(z) & 255;
    
    // Find relative x, z of point in square
    float xf = x - floor(x);
    float zf = z - floor(z);
    
    // Compute fade curves for each of x, z
    float u = smooth(xf);
    float v = smooth(zf);
    
    // Hash coordinates of the four corners of the square
    int A = (permutation[X] + Z) & 255;
    int B = (permutation[X + 1] + Z) & 255;
    int AA = permutation[A];
    int AB = permutation[(A + 1) & 255];
    int BA = permutation[B];
    int BB = permutation[(B + 1) & 255];
    
    // Compute gradient dot products for each corner
    float dotAA = grad(AA, xf,   zf);
    float dotBA = grad(BA, xf-1, zf);
    float dotAB = grad(AB, xf,   zf-1);
    float dotBB = grad(BB, xf-1, zf-1);
    
    // Interpolate along x then along z
    float lerpX1 = lerp(u, dotAA, dotBA);
    float lerpX2 = lerp(u, dotAB, dotBB);
    float res = lerp(v, lerpX1, lerpX2);
    
    // Map result to [0, 1] range
    return (res + 1.0f) / 2.0f;
}


float n_get(float x, float z, float freq, float amp, uint num_octaves) {
    float total = 0.0f;
    float max_value = 0.0f;

    for (int i = 0; i < num_octaves; i++) {
        total += perlin(x * freq, z * freq) * amp;
        max_value += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }

    return total / max_value;
} 

void n_init(uint seed) {
    for (int i = 0; i < 256; i++) {
        permutation[i] = i;
    }

    for (int i = 0; i < 256; i++) {
        srand(i + seed);
        permutation[i] = permutation[rand() % 256];
    }
}