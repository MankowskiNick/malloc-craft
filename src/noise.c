#include <noise.h>
#include <math.h>
// #include <stdlib.h>

static uint world_seed;

static int permutation[] = { 151,160,137,91,90,15,					// Hash lookup table as defined by Ken Perlin.  This is a randomly
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,	// arranged array of all numbers from 0-255 inclusive.
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

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

float n_get(float x, float z) {
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