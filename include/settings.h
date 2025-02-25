
#define WIDTH 800
#define HEIGHT 600

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 64

#define CHUNK_RENDER_DISTANCE 16
#define RENDER_DISTANCE 600.0f

// #define WIREFRAME

#define SEED 123456789

#define DELTA_X 0.2f
#define DELTA_Y 0.2f
#define DELTA_Z 0.2f
#define SENSITIVITY 0.001f

#define MAX_REACH 5.0f
#define RAY_STEP 0.05f

// unused as of now, but will be used to generate different biomes
#define WORLDGEN_BIOME_FREQUENCY 0.01f
#define WORLDGEN_BIOME_AMPLITUDE 0.05f
#define WORLDGEN_BIOME_OCTAVES 10

// used to generate the height of the terrain
#define WORLDGEN_BLOCKHEIGHT_FREQUENCY 0.25f
#define WORLDGEN_BLOCKHEIGHT_AMPLITUDE 2.0f
#define WORLDGEN_BLOCKHEIGHT_OCTAVES 6
#define WORLDGEN_BLOCKHEIGHT_MODIFIER 20.0f

#define CHUNK_CACHE_SIZE 2048