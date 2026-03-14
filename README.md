# malloc-craft

A Minecraft-like voxel game written in C using OpenGL, featuring procedural world generation, multiplayer networking, level-of-detail rendering, and a multithreaded architecture.

<img width="1195" alt="grafik" src="https://github.com/user-attachments/assets/e306427b-4685-433a-bbf4-f632135aa5b2" />
<img width="1195" alt="grafik" src="https://github.com/user-attachments/assets/126101ba-31f5-4e5f-aa1f-111a24711e28" />


## Key Features

### World Generation & Terrain
- Procedural terrain generation with Perlin noise
- Multiple biomes with distinct characteristics (configurable via `res/biomes.json`)
- Dynamic chunk loading and management with configurable render distance
- Procedural tree generation with JSON-based definitions
- Water physics with water level simulation and fluid flow mechanics

### Block System
- Extensive block types with JSON-based configuration (`res/blocks.json`)
- Support for opaque, transparent, liquid, and foliage block types
- Custom block models using Blockbench format
- Block placement and destruction with raycast-based selection
- Oriented blocks (blocks that can face different directions)

### Rendering & Graphics
- Texture atlas support for efficient block rendering
- Advanced water rendering with reflections and refraction
- Dynamic shadow mapping with softened shadows (PCF sampling)
- Screen-space reflections for water surfaces
- Liquid renderer with caustic effects
- Foliage renderer with alpha blending
- Skybox and sun rendering
- UI system with configurable FPS counter and hotbar display
- Wireframe mode for debugging

### Level of Detail (LOD)
- Dynamic LOD scaling based on player distance from each chunk
- Per-LOD chunk mesh caching — distant chunks use coarser geometry, reducing vertex count and draw calls
- Configurable via `lod_scaling_constant` and `max_lod_block_size` in `res/settings.json`

### Multiplayer & Networking
- Client-server architecture over TCP
- Chunk synchronization between server and all connected clients
- RLE compression for chunk data (~100x reduction on typical terrain)
- Broadcast system for real-time world updates to multiple clients
- Headless server mode — run without graphics using the `--server` flag
- Thread-local socket pool for zero-lock socket access per thread
- Configurable host, port, and world storage directory in `res/settings.json`

### Performance & Architecture
- Worker pool with configurable thread count for parallel chunk mesh generation
- Mesh batching for efficient GPU utilization
- Chunk-based world organization
- Configurable graphics settings (mipmapping, atlas size, tick rate)

## Multithreaded Architecture

malloc-craft uses a multithreaded architecture to keep gameplay smooth while handling chunk loading, mesh generation, and networked updates concurrently.

### Thread Roles

**Client-side threads:**
- **Main/Render Thread:** Handles rendering, input, and overall game loop. Updates game state and coordinates mesh updates.
- **Worker Pool (N threads):** Parallel chunk mesh generation. The number of worker threads is configurable via `worker_threads` in `res/settings.json` (default: 8). See `src/mesh/core/worker_pool.c`.
- **Chunk Mesh Updater Thread:** Dispatches dirty chunks to the worker pool and collects results, separating blocks into opaque, transparent, liquid, and foliage layers. See `src/mesh/generation/chunk_mesh.c` (`start_chunk_mesh_updater`).
- **World Mesh Updater Thread:** Periodically rebuilds the combined world mesh from individual chunk meshes for efficient rendering. See `src/mesh/generation/world_mesh.c` (`start_world_mesh_updater`).

**Server-side threads:**
- **Listener Thread:** Accepts incoming client connections.
- **Per-Client Receive Thread:** One thread per connected client, reads incoming messages.
- **Broadcast Thread:** Distributes world updates to all connected clients from a shared queue.

### Synchronization
Shared data such as mesh buffers, game state, and camera position are protected using mutexes to ensure thread safety and prevent race conditions. Key synchronization:
- `lock_mesh()` / `unlock_mesh()` – Protect mesh data during updates
- Thread-local socket pool (`pthread_key_t`) – Per-thread sockets with no locking needed
- Game data is shared via the `game_data` structure with proper locking mechanisms

### Data Flow
1. Main thread updates player camera and detects chunk changes
2. Chunk mesh updater dispatches dirty chunks to the worker pool
3. Worker threads generate per-chunk meshes in parallel (opaque, transparent, liquid, foliage, LOD variants)
4. World mesh updater combines completed chunk meshes into batched world mesh packets
5. Main render thread retrieves updated meshes and renders to screen
6. On multiplayer servers, chunk updates are also broadcast to all connected clients via the broadcast queue

### Mesh Stratification
Meshes are organized into separate buffers by rendering layer:
- **Opaque:** Solid blocks (stone, dirt, etc.)
- **Transparent:** Blocks with alpha (glass, etc.)
- **Liquid:** Water blocks with flow simulation
- **Foliage:** Grass and plant-like blocks with special rendering
- **Custom Models:** Blockbench-based decorative models

### Extending & Debugging
When adding new threaded features:
- Always use proper mutex locking to protect shared data
- Watch for potential deadlocks and race conditions
- Test with multiple chunks loading/unloading simultaneously
- Use frame time monitoring to ensure threading isn't causing stalls
- Consider thread pool patterns for many-to-one operations

### Code References
- `src/mesh/generation/chunk_mesh.c` – Chunk mesh generation
- `src/mesh/generation/world_mesh.c` – World mesh aggregation
- `src/mesh/core/mesh.c` – Mesh data structure and LOD caching
- `src/mesh/core/worker_pool.c` – Worker thread pool
- `include/game_data.h` – Shared game state

## Multiplayer

malloc-craft supports a dedicated server mode that allows multiple clients to connect and share the same world.

### Running a Server
```sh
./malloc-craft --server
```
The server runs headlessly (no window or graphics context). Clients connect using the host and port configured in `res/settings.json`.

### Architecture
- The server maintains an in-memory chunk cache (up to 1024 chunks) with disk eviction for overflow
- Chunk data is RLE-compressed before transmission: a typical chunk compresses from ~12KB to ~120 bytes
- The server relays compressed chunks directly to clients without decompressing (broadcast-without-decompress)
- Each client gets its own receive thread; outgoing updates go through a single broadcast queue

### Configuration (`res/settings.json`)
```json
"server": {
    "port": 8085,
    "host": "192.168.4.141",
    "worlds_dir": "./worlds/"
}
```

### Wire Format
Chunk packets contain: `x (4B) | z (4B) | packet_count (4B)` followed by runs of `block_data (3B) | run_length (4B)`.

For an in-depth look at the networking internals, see [`MULTIPLAYER_DEEP_DIVE.md`](MULTIPLAYER_DEEP_DIVE.md).

## Configuration & Settings

malloc-craft is highly configurable through JSON files in the `res/` directory:

### Display Settings (`res/settings.json`)
- **Resolution & Display:** Window dimensions, fullscreen mode, V-sync, FOV
- **Chunk Management:** Per-frame chunk load rate, cache size, render distance, worker thread count
- **LOD:** `lod_scaling_constant`, `max_lod_block_size` — control when and how aggressively LOD kicks in
- **Graphics:** Wireframe mode, atlas size, mipmapping, tick rate
- **Textures:** Atlas, bump map, skybox, and caustic texture paths
- **Server:** Host, port, and world storage directory

### Game Configuration
- **Blocks** (`res/blocks.json`): Block types, textures, properties (transparent, liquid, foliage, oriented)
- **Biomes** (`res/biomes.json`): Biome types with surface/subsurface/underground/underwater block types and foliage distribution
- **Trees** (`res/trees.json`): Procedural tree generation rules and variations
- **UI** (`res/ui.json`): HUD layout including FPS counter position and hotbar styling
- **Player** (`res/player.json`): Default player spawn state

For detailed settings information, see the JSON files directly.

## Project Structure

```
src/
├── render/              # Graphics rendering subsystem
│   ├── core/           # Shader, texture, window management
│   ├── geometry/       # VAO/VBO (vertex buffer management)
│   ├── effects/        # Shadow maps, reflections, frame buffers
│   ├── world/          # Block, liquid, and foliage renderers
│   ├── entities/       # Custom model and outline rendering
│   ├── environment/    # Skybox and sun rendering
│   └── ui/             # UI renderer for HUD elements
├── mesh/                # Mesh generation and management
│   ├── core/           # Mesh data structures, LOD caching, worker pool
│   ├── generation/     # Chunk mesh and world mesh builders
│   └── geometry/       # Blockbench model loading
├── player/              # Player logic and input handling
│   ├── core/           # Camera and player state
│   ├── input/          # Keyboard/mouse input processing
│   └── serialization/  # Player data persistence
├── world/               # World simulation
│   ├── core/           # Chunks, blocks, world state
│   ├── generation/     # Biome, terrain, and tree generation
│   └── physics/        # Water physics and fluid flow
├── server/              # Multiplayer server
│   ├── server.c        # Server entry point and socket setup
│   ├── client.c        # Client connection management, thread-local socket pool
│   ├── threads/        # Listener, broadcast, and per-client receive threads
│   ├── world/          # Server-side world state and chunk disk I/O
│   └── compression/    # RLE chunk compression/decompression
├── util/                # Utility functions and settings
└── glad/                # OpenGL loader (auto-generated)

include/
├── cglm/                # Math library headers
├── GLFW/                # Window/input library headers
├── stb/                 # Image loading headers
├── cerialize/           # Custom serialization utilities
├── game_data.h          # Core game state structure
├── hashmap.h            # Hash map implementation
├── block_models.h       # Blockbench model definitions
└── util.h               # Utility function declarations

lib/
├── cglm/                # Math library for graphics (submodule)
├── GLFW/                # Window/input handling (submodule)
└── glad/                # OpenGL loader (included)

res/
├── shaders/             # GLSL vertex and fragment shaders
│   ├── world/          # world, liquid, foliage, blockbench shaders
│   ├── effects/        # shadow and reflection shaders
│   ├── environment/    # skybox and sun shaders
│   └── ui/             # outline and HUD shaders
├── textures/            # Game textures
├── blocks.json          # Block type definitions
├── biomes.json          # Biome configurations
├── trees.json           # Tree generation rules
├── ui.json              # UI layout and styling
├── settings.json        # Engine configuration
├── player.json          # Default player state
└── blockbench/          # Custom block models
```

## Prerequisites

- **CMake** (version 3.10 or higher)
- **C Compiler** with C11 support (gcc or clang)
- **OpenGL 3.3+** compatible graphics hardware and drivers
- **POSIX Threads** (pthread) support for multithreading

## Building

### Clone with Submodules
```sh
git clone --recurse-submodules https://github.com/MankowskiNick/malloc-craft.git
cd malloc-craft
```

### Configure and Build
```sh
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Run
```sh
# Single-player
./malloc-craft

# Dedicated server (headless)
./malloc-craft --server
```

## Platform Support

- **Linux:** Fully supported (developed and tested on Linux/macOS)
- **macOS:** Fully supported (developed and tested on Linux/macOS)
- **Windows:** Unsupported

## Usage

| Key | Action |
|-----|--------|
| WASD | Move |
| Space | Jump |
| Left Shift | Crouch |
| Double-tap W | Sprint |
| Mouse | Look around |
| Left Click | Destroy block |
| Right Click | Place block |
| 0–9 | Select from hotbar |
| V | Toggle no-clip / fly mode |
| F1 | Toggle FPS counter |
| F11 | Toggle fullscreen |
| ESC | Exit |

## Acknowledgments

- [GLFW](https://www.glfw.org/) – Window and input management
- [cglm](https://github.com/recp/cglm) – Graphics math library
- [stb](https://github.com/nothings/stb/) – Image loading (stb_image)
- [glad](https://github.com/Dav1dde/glad) – OpenGL loading library

## Development & Architecture

### Core Concepts

**Chunks:** The world is divided into chunks (16×256×16 blocks). Each chunk maintains its own mesh data per LOD level and can be loaded/unloaded independently.

**Level of Detail:** Each chunk stores multiple cached meshes at different LOD levels. The active LOD is selected at render time based on the chunk's distance from the player, reducing geometry for far-away chunks automatically.

**Meshes:** Three levels of mesh organization:
- Individual chunk meshes (per-chunk vertex data, one per LOD level)
- World mesh packets (grouped chunks for efficient rendering)
- Stratified rendering layers (opaque, transparent, liquid, foliage)

**Game Data:** All shared state is managed through the `game_data` structure, including:
- Current camera position and chunk coordinates
- All loaded chunk meshes (with LOD cache)
- World mesh packets
- Player reference
- Frame timing and statistics

### Adding New Features

1. **New Block Types:** Add JSON definitions to `res/blocks.json` and corresponding textures to the atlas
2. **New Shaders:** Add `.vert` and `.frag` files to the appropriate `res/shaders/` subdirectory, then reference in the relevant renderer
3. **New Biome:** Add to `res/biomes.json` with foliage and block types
4. **Custom Models:** Create in Blockbench, export JSON, add to `res/blockbench/`, and reference in block definitions
5. **New Renderers:** Create new renderer module in `src/render/`, implement shader compilation and rendering logic
6. **Multiplayer Messages:** Add new message types in `src/server/`, handle on both the listen and broadcast threads
