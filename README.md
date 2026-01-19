# malloc-craft
[GitHub Repository](https://github.com/MankowskiNick/malloc-craft)

A Minecraft-like voxel game written in C using OpenGL. This project focuses on memory management and modern graphics programming techniques to create a block-based 3D world where players can build and explore.

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
- Dynamic shadow mapping with softened shadows
- Screen-space reflections for water surfaces
- Liquid renderer with caustic effects
- Foliage renderer with alpha blending
- Skybox and sun rendering
- UI system with configurable FPS counter and hotbar display
- Wireframe mode for debugging

### Performance & Architecture
- Mesh batching for efficient GPU utilization
- Multithreaded mesh generation (separate chunk and world mesh updaters)
- Chunk-based world organization
- Frustum culling for visibility optimization
- Configurable graphics settings (mipmapping, atlas size, tick rate)

## Multithreaded Architecture

malloc-craft uses a multithreaded architecture to keep gameplay smooth and responsive, especially when updating chunk and world meshes. Mesh updates are performed in background threads, allowing rendering and input to remain fast even as the world changes.

### Thread Roles
- **Main/Render Thread:** Handles rendering, input, and overall game loop. Updates game state and coordinates mesh updates.
- **Chunk Mesh Updater Thread:** Generates individual chunk meshes in the background, separating blocks into opaque, transparent, liquid, and foliage layers. See `src/mesh/generation/chunk_mesh.c` (`start_chunk_mesh_updater`).
- **World Mesh Updater Thread:** Periodically rebuilds the combined world mesh from individual chunk meshes for efficient rendering. See `src/mesh/generation/world_mesh.c` (`start_world_mesh_updater`).

### Synchronization
Shared data such as mesh buffers, game state, and camera position are protected using mutexes to ensure thread safety and prevent race conditions. Key synchronization functions:
- `lock_mesh()` / `unlock_mesh()` – Protect mesh data during updates
- Game data is shared via the `game_data` structure with proper locking mechanisms

### Data Flow
1. Main thread updates player camera and detects chunk changes
2. Chunk mesh updater detects modified chunks and generates per-chunk meshes (opaque, transparent, liquid, foliage)
3. World mesh updater combines chunk meshes into batched world mesh
4. Main render thread retrieves updated meshes and renders to screen
5. Synchronization points ensure consistent state across all threads

### Mesh Stratification
Meshes are organized into separate buffers by rendering layer:
- **Opaque:** Solid blocks (stone, dirt, etc.)
- **Transparent:** Blocks with alpha (glass, water, etc.)
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
- `src/mesh/core/mesh.c` – Mesh data structure management
- `include/game_data.h` – Shared game state

## Configuration & Settings

malloc-craft is highly configurable through JSON files in the `res/` directory:

### Display Settings (`res/settings.json`)
- **Resolution & Display:** Window dimensions, fullscreen mode, V-sync, FOV
- **Chunk Management:** Per-frame chunk load rate, cache size, render distance
- **Graphics:** Wireframe mode, render distance, atlas size, mipmapping, tick rate
- **Textures:** Atlas, bump map, skybox, and caustic texture paths

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
│   ├── core/           # Mesh data structures
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
│   ├── world.vert/.frag      # Main world rendering
│   ├── liquid.vert/.frag     # Water rendering
│   ├── foliage.vert/.frag    # Grass/plant rendering
│   ├── shadow.vert/.frag     # Shadow depth map
│   ├── reflection.vert/.frag # Water reflections
│   ├── skybox.vert/.frag     # Skybox rendering
│   ├── sun.vert/.frag        # Sun rendering
│   ├── ui.vert/.frag         # UI rendering
│   ├── outline.vert/.frag    # Block outline selection
│   └── blockbench.vert/.frag # Custom model rendering
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
- **C Compiler** with C11 support (gcc, clang, MSVC)
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
./malloc-craft
```

## Platform Support

- **Linux:** Fully supported (developed and tested on Linux/MacOS)
- **macOS:** Fully supported (developed and tested on Linux/MacOS)
- **Windows:** Unsupported

## Usage

- **WASD:** Move
- **Space:** Jump
- **Mouse:** Look around
- **Left Click:** Destroy block
- **Right Click:** Place block
- **0-9:** Select from inventory
- **V:** No clip mode
- **F1:** Toggle FPS counter
- **F11:** Toggle fullscreen
- **ESC:** Exit

## Acknowledgments

- [GLFW](https://www.glfw.org/) – Window and input management
- [cglm](https://github.com/recp/cglm) – Graphics math library
- [stb](https://github.com/nothings/stb/) – Image loading (stb_image)
- [glad](https://github.com/Dav1dde/glad) – OpenGL loading library

## Development & Architecture

### Core Concepts

**Chunks:** The world is divided into chunks (typically 16×256×16 blocks). Each chunk maintains its own mesh data and can be loaded/unloaded independently.

**Meshes:** Three levels of mesh organization:
- Individual chunk meshes (per-chunk vertex data)
- World mesh packets (grouped chunks for efficient rendering)
- Stratified rendering layers (opaque, transparent, liquid, foliage)

**Game Data:** All shared state is managed through the `game_data` structure, including:
- Current camera position and chunk coordinates
- All loaded chunk meshes
- World mesh packets
- Player reference
- Frame timing and statistics

### Adding New Features

1. **New Block Types:** Add JSON definitions to `res/blocks.json` and corresponding textures to the atlas
2. **New Shaders:** Add `.vert` and `.frag` files to `res/shaders/`, then reference in appropriate renderer
3. **New Biome:** Add to `res/biomes.json` with foliage and block types
4. **Custom Models:** Create in Blockbench, export JSON, add to `res/blockbench/`, and reference in block definitions
5. **New Renderers:** Create new renderer module in `src/render/`, implement shader compilation and rendering logic