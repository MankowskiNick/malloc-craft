
#define WIDTH 800
#define HEIGHT 600

#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256

#define CHUNK_RENDER_DISTANCE 4
#define RENDER_DISTANCE CHUNK_RENDER_DISTANCE * CHUNK_SIZE

// #define WIREFRAME

// mipmap tends to make things slower
// #define USE_MIPMAP

#define DELTA_X 0.2f
#define DELTA_Y 0.2f
#define DELTA_Z 0.2f
#define SENSITIVITY 0.001f

#define MAX_REACH 5.0f
#define RAY_STEP 0.05f