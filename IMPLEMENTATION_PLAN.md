# Multi-Worker Thread Chunk Loading Implementation Plan

## Current Architecture Overview

The project currently uses a **single worker thread** for chunk mesh generation:
- **Chunk Mesh Updater Thread** (`start_chunk_mesh_updater` in `chunk_mesh.c`): Generates individual chunk meshes by calling `get_chunk_meshes()` in a loop with `usleep(TICK_RATE)` delays
- **World Mesh Updater Thread** (separate, in `world_mesh.c`): Combines chunk meshes into world-space meshes
- **Main Render Thread**: Manages rendering, input, and game loop

### Current Threading Flow
```
Main Thread
    ↓
Chunk Mesh Updater (1 thread)
    ↓
get_chunk_meshes() → Updates all render-distance chunks each frame
    ├── Calls get_chunk_mesh() for each chunk
    ├── Calls load_chunk() to load queued chunks from chunk_load_queue
    ├── Performs sorting and LOD updates
    └── Returns array of chunk meshes
    ↓
World Mesh Updater (1 thread)
    ↓
Render Output
```

### Current Bottleneck
- All mesh generation happens in **one thread**
- `get_chunk_meshes()` is CPU-bound and must process all chunks sequentially
- Higher `CHUNK_RENDER_DISTANCE` creates longer processing times
- Single thread cannot utilize multi-core systems effectively

---

## Implementation Plan

### Phase 1: Configuration & Settings
**Files to modify:** `res/settings.json`, `src/util/settings.h`, `src/util/settings.c`

1. **Add new setting to `res/settings.json`:**
   ```json
   "chunks": {
     "worker_threads": 4,        // NEW: Number of worker threads for chunk loading
     "chunk_load_per_frame": 1,
     "chunk_cache_size": 1024,
     "chunk_render_distance": 6,
     "lod_scaling_constant": 2
   }
   ```

2. **Create extern variable in `settings.h`:**
   ```c
   extern int WORKER_THREADS;
   ```

3. **Parse setting in `settings.c`:**
   - Add parsing logic in `parse_chunks_settings()`
   - Default to 4 if not specified (reasonable for modern systems)
   - Validate range: 1-16 threads (cap to prevent resource exhaustion)

---

### Phase 2: Worker Thread Pool Architecture
**Files to create:** `src/mesh/core/worker_pool.h`, `src/mesh/core/worker_pool.c`

Create a reusable thread pool manager to handle:
- Thread creation/destruction
- Work queue management
- Synchronization primitives
- Lifecycle management

**Key structures:**
```c
typedef struct {
    pthread_t thread_id;
    int worker_id;
    int is_active;
} worker_thread;

typedef struct {
    worker_thread* threads;
    int num_threads;
    queue_node* work_queue;           // Queue of chunk coordinates to process
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_signal;      // Signal when work is available
    int shutdown_flag;
} worker_pool;
```

**Functions:**
- `worker_pool* pool_init(int num_threads)` - Create pool with N threads
- `void pool_submit_work(worker_pool* pool, void* work_item)` - Queue work
- `void pool_shutdown(worker_pool* pool)` - Stop all threads gracefully
- `void pool_wait_idle(worker_pool* pool)` - Wait for queue to empty

---

### Phase 3: Chunk Work Distribution
**Files to modify:** `src/mesh/core/mesh.c`, `src/mesh/generation/chunk_mesh.c`, `src/mesh/generation/chunk_mesh.h`

1. **Create chunk work item structure:**
   ```c
   typedef struct {
       int x, z;
       float player_x, player_z;
       // Result storage
       chunk_mesh* result_mesh;
       int work_complete;
   } chunk_work_item;
   ```

2. **Refactor chunk mesh generation:**
   - Extract `create_chunk_mesh()` logic into `process_chunk_work_item()`
   - Worker threads execute this function on queue items
   - Store results in work item and push to results queue

3. **Results aggregation:**
   - Main chunk updater thread collects completed work from results queue
   - Still responsible for sorting/LOD updates on the aggregated results
   - Maintains same synchronization with world mesh updater

---

### Phase 4: Worker Thread Main Loop
**Implementation in `worker_pool.c`**

Each worker thread executes:
```c
void* worker_thread_main(void* arg) {
    worker_thread* self = (worker_thread*)arg;
    worker_pool* pool = (worker_pool*)self->pool;
    
    while (!pool->shutdown_flag) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        // Wait for work or shutdown signal
        while (queue_is_empty(pool->work_queue) && !pool->shutdown_flag) {
            pthread_cond_wait(&pool->queue_signal, &pool->queue_mutex);
        }
        
        if (pool->shutdown_flag) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        chunk_work_item* work = (chunk_work_item*)queue_pop(&pool->work_queue);
        pthread_mutex_unlock(&pool->queue_mutex);
        
        if (work) {
            // Process chunk mesh generation
            work->result_mesh = create_chunk_mesh(work->x, work->z, work->player_x, work->player_z);
            work->work_complete = 1;
            
            // Push to results queue
            pthread_mutex_lock(&pool->results_mutex);
            queue_push(&pool->results_queue, work, NULL);
            pthread_mutex_unlock(&pool->results_mutex);
        }
    }
    
    return NULL;
}
```

---

### Phase 5: Integration with Existing System
**Files to modify:** `src/mesh/generation/chunk_mesh.c`, `src/main.c`

1. **Modify `get_chunk_meshes()`:**
   - Instead of directly calling `create_chunk_mesh()` in a loop
   - Create `chunk_work_item` structures for each chunk
   - Submit to worker pool using `pool_submit_work()`
   - Collect results from results queue
   - Wait for all submitted work to complete before returning

2. **Update `start_chunk_mesh_updater()`:**
   - Initialize worker pool with `WORKER_THREADS` count
   - Maintain the same outer loop structure
   - Control flow remains similar from caller's perspective

3. **Modify `main.c`:**
   - Initialize worker pool before starting mesh updater
   - Pass pool reference to updater thread
   - Clean up pool on shutdown

---

### Phase 6: Synchronization & Data Safety

**Protect shared data:**
- `chunk_packets` hashmap: Already protected by `chunk_packets_mutex`
- Worker pool queue: Protected by `queue_mutex` + `queue_signal`
- Results queue: New mutex `results_queue_mutex`
- Camera cache: Still updated by main chunk updater thread only

**Thread safety strategies:**
1. **Read-only sharing:** Block data accessed by workers is read-only (no conflicts)
2. **Queue-based communication:** Decouples work submission from completion
3. **Conditional variables:** Efficient worker wake-up when work available
4. **Atomic flags:** `work_complete` and `shutdown_flag` for simple synchronization

---

### Phase 7: Load Balancing (Optional Enhancement)

**Future consideration:** Work distribution strategy can be optimized:
- **Round-robin:** Distribute chunks evenly across threads (current plan)
- **Work stealing:** Idle threads can steal from busier threads
- **Priority queue:** Prioritize chunks closer to player
- **Adaptive threading:** Adjust thread count based on queue depth

---

## Modified File Structure

### New Files
```
src/mesh/core/worker_pool.h        # Worker pool interface
src/mesh/core/worker_pool.c        # Worker pool implementation
```

### Modified Files
```
res/settings.json                  # Add WORKER_THREADS setting
src/util/settings.h                # Add extern WORKER_THREADS
src/util/settings.c                # Parse WORKER_THREADS setting
src/mesh/core/mesh.h               # Add worker pool forward declaration
src/mesh/core/mesh.c               # Initialize/cleanup worker pool
src/mesh/generation/chunk_mesh.h   # Update function signatures if needed
src/mesh/generation/chunk_mesh.c   # Refactor to use worker pool
src/main.c                         # Initialize worker pool in startup
include/game_data.h                # May add worker pool reference to game_data
```

---

## Implementation Order

1. ✅ **Create worker pool abstraction** (`worker_pool.h/c`)
2. ✅ **Add configuration** (settings files)
3. ✅ **Refactor chunk work** (work items, process function)
4. ✅ **Integrate worker pool** (mesh.c, chunk_mesh.c)
5. ✅ **Test with 1, 2, 4, 8 threads** to find optimal count
6. ✅ **Profile and tune** mutex contention, work distribution
7. ⚠️ **Optional: Add work prioritization** by distance from player
8. ⚠️ **Optional: Implement adaptive threading**

---

## Expected Improvements

| Metric | Before | After (4 threads) |
|--------|--------|-------------------|
| Chunk mesh generation time | 100% | ~40-60% (depending on CPU cores) |
| Multi-core utilization | ~25% | ~75%+ |
| Stutter reduction | High for high LOD distances | Reduced |
| Configuration flexibility | Fixed | User-adjustable (1-16 threads) |

---

## Risk Considerations

1. **Lock contention:** Multiple threads accessing `chunk_packets` hashmap
   - *Mitigation:* Fine-grained locking, read-write locks if needed

2. **Memory overhead:** Each thread has stack + local data
   - *Mitigation:* Cap at 16 threads maximum

3. **Excessive work queue depth:** Too much queued work
   - *Mitigation:* Backpressure mechanism if queue grows too large

4. **Uneven load distribution:** Some chunks take longer to process
   - *Mitigation:* Monitor queue depth, implement work stealing if needed

---

## Testing Strategy

1. **Unit tests:** Verify worker pool create/destroy/submit
2. **Stress tests:** Create 1000+ queued chunks, verify all complete
3. **Performance benchmarks:** Compare 1, 2, 4, 8 thread configurations
4. **Profiling:** Use `perf` or `Instruments` to find bottlenecks
5. **Race condition testing:** Run with thread sanitizer

---

## Backwards Compatibility

- Existing code remains functional with `WORKER_THREADS = 1`
- Default to reasonable value (4 threads) but adjustable
- No breaking changes to public APIs
