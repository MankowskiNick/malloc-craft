
// Screen width & height
#define WIDTH 800
#define HEIGHT 600
#define TITLE "malloc-craft"




// CHUNK SETTINGS

// Chunk size
#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

// How many chunks can be loaded per frame.  
// Higher values will load chunks faster, but may cause stuttering.
#define CHUNK_LOAD_PER_FRAME 1

// Chunks are cached in memory to reduce load times, how large should the cache be?
#define CHUNK_CACHE_SIZE 1024

// Radius of chunks to redner
#define CHUNK_RENDER_DISTANCE 20

// Render distance - also serves as the fog distance
#define RENDER_DISTANCE 500.0f





// GRAPHICS SETTINGS 

// Render as wireframe? Useful for debugging
// #define WIREFRAME

// Vsync on?  Comment to disable
#define VSYNC

// Allow transparent leaves?  Transparent leaves can negatively impact performance.
// Comment to disable
#define TRANSPARENT_LEAVES

// Atlas size - the texture atlas is a spritemap that contains 
// ATLAS_SIZE x ATLAS_SIZE textures.
#define ATLAS_SIZE 32

// Use mipmaps?  Comment to disable
#define USE_MIPMAP

// Skybox settings
#define SKYBOX_SLICES 15
#define SKYBOX_STACKS 15
#define SKYBOX_RADIUS 1.0f

// Water settings
#define WATER_OFFSET 0.2f
#define WATER_HEIGHT 1.0f





// PLAYER SETTINGS

// Camera settings
#define DELTA_X 0.1f
#define DELTA_Y 0.1f
#define DELTA_Z 0.1f
#define SENSITIVITY 0.001f

// Player settings
#define MAX_REACH 5.0f
#define RAY_STEP 0.05f





// WORLD GENERATION SETTINGS

// World generation seed
#define SEED 42069

// Perlin noise settings for biome generation
#define WORLDGEN_BIOME_FREQUENCY 0.2f
#define WORLDGEN_BIOME_AMPLITUDE 1.0f
#define WORLDGEN_BIOME_OCTAVES 2

// Perlin noise is generated and normalized to [-1, 1].  
// This value is the modifier to scale the height
#define WORLDGEN_BLOCKHEIGHT_MODIFIER 40.0f
#define WORLDGEN_BASE_TERRAIN_HEIGHT 64 // The height of the base terrain
#define WORLDGEN_WATER_LEVEL 64 // The height of the water level

// Perlin noise settings for block height generation
#define WORLDGEN_BLOCKHEIGHT_FREQUENCY 0.25f
#define WORLDGEN_BLOCKHEIGHT_AMPLITUDE 2.0f
#define WORLDGEN_BLOCKHEIGHT_OCTAVES 6

// Biome, tree, and block counts
#define BIOME_COUNT 4
#define TREE_COUNT 2
#define BLOCK_COUNT 13