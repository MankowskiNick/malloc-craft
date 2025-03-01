
#define WIDTH 800
#define HEIGHT 600

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

#define CHUNK_RENDER_DISTANCE 16
#define RENDER_DISTANCE 600.0f

#define VSYNC
#define TRANSPARENT_LEAVES
#define CHUNK_LOAD_PER_FRAME 3

#define WIREFRAME

// #define USE_MIPMAP

#define SEED 42069

#define DELTA_X 0.1f
#define DELTA_Y 0.1f
#define DELTA_Z 0.1f
#define SENSITIVITY 0.001f

#define MAX_REACH 5.0f
#define RAY_STEP 0.05f

// how many textures are packed into one side of the atlas
#define TEXTURE_ATLAS_SIZE 32

// unused as of now, but will be used to generate different biomes
#define WORLDGEN_BIOME_FREQUENCY 0.1f
#define WORLDGEN_BIOME_AMPLITUDE 1.0f
#define WORLDGEN_BIOME_OCTAVES 2

#define WORLDGEN_WATER_LEVEL 84

// used to generate the height of the terrain
#define WORLDGEN_BLOCKHEIGHT_FREQUENCY 0.25f
#define WORLDGEN_BLOCKHEIGHT_AMPLITUDE 2.0f
#define WORLDGEN_BLOCKHEIGHT_OCTAVES 6
#define WORLDGEN_BLOCKHEIGHT_MODIFIER 40.0f
#define WORLDGEN_BASE_TERRAIN_HEIGHT 64

#define BIOME_COUNT 4
#define TREE_COUNT 2

#define CHUNK_CACHE_SIZE 2048