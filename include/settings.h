#ifndef SETTINGS_H
#define SETTINGS_H


void read_settings(const char* filename);

// Screen width & height
extern int WIDTH;
extern int HEIGHT;
extern char* TITLE;

extern float TIME_SCALE;

// CHUNK SETTINGS

// Chunk size
#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256
// How many chunks can be loaded per frame.  
// Higher values will load chunks faster, but may cause stuttering.
extern int CHUNK_LOAD_PER_FRAME;

// Chunks are cached in memory to reduce load times, how large should the cache be?
extern int CHUNK_CACHE_SIZE;





 // GRAPHICS SETTINGS 

// Render as wireframe? Useful for debugging
extern int WIREFRAME;

extern char* ATLAS_PATH;
extern char* BUMP_PATH;
extern char* SKYBOX_PATH;
extern char* CAUSTIC_PATH;

extern int ATLAS_TEXTURE_INDEX;
extern int BUMP_TEXTURE_INDEX;
extern int CAUSTIC_TEXTURE_INDEX;
extern int SKYBOX_TEXTURE_INDEX;
extern int SHADOW_MAP_TEXTURE_INDEX;
extern int REFLECTION_MAP_TEXTURE_INDEX;

// Vsync on?  Comment to disable
extern int VSYNC;
// Radius of chunks to render
extern int CHUNK_RENDER_DISTANCE;

// Shadow map settings
extern int SHADOW_MAP_WIDTH;
extern int SHADOW_MAP_HEIGHT;
extern float SHADOW_RENDER_DIST;
extern float SHADOW_SOFTNESS;
extern int SHADOW_SAMPLES;
extern float SHADOW_BIAS;

// Reflection Settings
extern int REFLECTION_FBO_WIDTH;
extern int REFRACTION_FBO_HEIGHT;


 // How often to update the world mesh in seconds
extern int TICK_RATE; // 32ms is roughly 30 updates per second

// FOV
extern float FOV;

// Render distance - also serves as the fog distance
extern float RENDER_DISTANCE;

// Atlas size - the texture atlas is a spritemap that contains 
// ATLAS_SIZE x ATLAS_SIZE textures.
extern int ATLAS_SIZE;

// Use mipmaps?  Comment to disable
extern int USE_MIPMAP;

 // Skybox settings
extern int SKYBOX_SLICES;
extern int SKYBOX_STACKS;
extern float SKYBOX_RADIUS;

// Sun settings
extern int SUN_SLICES;
extern int SUN_STACKS;
extern float SUN_RADIUS;
extern float SUN_INTENSITY;
extern float SUN_SPECULAR_STRENGTH;
extern float WATER_SHININESS;

// Ambient light
extern float AMBIENT_R_INTENSITY;
extern float AMBIENT_G_INTENSITY;
extern float AMBIENT_B_INTENSITY;

// Water settings
extern float WATER_OFFSET;
extern float WATER_HEIGHT;
extern float WATER_DISTANCE;

// PLAYER SETTINGS

 // Camera settings
extern float DELTA_X;
extern float DELTA_Y;
extern float DELTA_Z;
extern float SENSITIVITY;

// Player settings
extern float MAX_REACH;
extern float RAY_STEP;





// WORLD GENERATION SETTINGS

// World generation seed
extern int SEED;

// Perlin noise settings for biome generation
extern float WORLDGEN_BIOME_FREQUENCY;
extern float WORLDGEN_BIOME_AMPLITUDE;
extern int WORLDGEN_BIOME_OCTAVES;

// Perlin noise is generated and normalized to [-1, 1].  
// This value is the modifier to scale the height
extern float WORLDGEN_BLOCKHEIGHT_MODIFIER;
extern int WORLDGEN_BASE_TERRAIN_HEIGHT; // The height of the base terrain
extern int WORLDGEN_WATER_LEVEL; // The height of the water level

// Perlin noise settings for block height generation
extern float WORLDGEN_BLOCKHEIGHT_FREQUENCY;
extern float WORLDGEN_BLOCKHEIGHT_AMPLITUDE;
extern int WORLDGEN_BLOCKHEIGHT_OCTAVES;

// Biome, tree, and block counts
extern int BIOME_COUNT; // TODO: remove some of these counts so they are automatically calculated
extern int TREE_COUNT;
extern int BLOCK_COUNT;
extern char* BLOCK_FILE;
extern char* PLAYER_FILE;

#endif
