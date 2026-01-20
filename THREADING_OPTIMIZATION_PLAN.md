# Worker Thread Optimization Plan: Reduce Lock Contention

## Problem Analysis

Your current threading model has these bottlenecks:

### Current Architecture Issues:
1. **Long-lived mutex in `chunk_mesh.c`**: `cm_mutex` is held during `get_chunk_meshes()` which includes sorting and packaging - expensive operations
2. **World mesh creation is blocking**: `create_world_mesh()` assembles all chunk data while holding locks, blocking render thread access
3. **Shared mutable state**: Multiple threads contend for `chunk_packets_mutex`, `sort_queue_mutex`, `chunk_load_queue_mutex`
4. **Two-pass approach**: World mesh rebuilds iterate chunk meshes twice (calculate size, then copy) with locks held

### Performance Impact:
- Main render thread stalls waiting for mesh locks
- Worker threads wait for locks during packet updates
- Large datasets amplify contention (high render distance = long lock times)

---

## Solution: Copy-on-Update Pattern

Use **double-buffering** and **thread-local copies** to minimize critical sections:

### Key Principles:
1. **Workers write to private copies** - No lock needed during expensive operations
2. **Atomic swaps** - Brief, lock-protected data swaps (O(1) pointer assignment)
3. **Deferred cleanup** - Free old data after safe swap point
4. **Isolated synchronization** - Each subsystem (chunk worker, world mesh worker) has independent buffering

---

## Implementation Plan

### Phase 1: Chunk Mesh Worker Optimization
**Goal**: Reduce lock time in chunk packet updates from seconds → milliseconds

#### Step 1.1: Add Double-Buffered Packet Storage
**File**: `src/mesh/core/mesh.c` (modify initialization)

```c
// Current:
chunk_mesh_map chunk_packets;
pthread_mutex_t chunk_packets_mutex;

// New - double-buffered approach:
chunk_mesh_map chunk_packets;           // Active (read-only during swap)
chunk_mesh_map chunk_packets_buffer;    // Worker thread writes here
pthread_mutex_t packet_swap_mutex;      // Only protects swap operation
pthread_cond_t packet_update_signal;    // Signal render thread about updates
```

**Rationale**: Workers build entire new packet map in `chunk_packets_buffer`, then swap pointers atomically. Render thread reads from stable `chunk_packets` most of the time.

#### Step 1.2: Refactor `get_chunk_meshes()` into Two Phases
**File**: `src/mesh/generation/chunk_mesh.c`

**Before**:
```
lock_mesh()
  // Build chunk list
  // Sort by camera distance  
  // Package data
unlock_mesh()
```

**After - Phase 1 (No Lock)**:
```
// Build in thread-local buffer
build_chunk_mesh_array_into_buffer(buffer)
// Sort locally
quicksort(buffer, ...)
// Package locally into new map
package_into_new_map(buffer, new_map)
```

**Phase 2 (Brief Lock)**:
```
lock_packet_swap_mutex()
  old_packets = chunk_packets
  chunk_packets = new_map    // Pointer swap - nanoseconds
  broadcast_packet_update_signal()
unlock_packet_swap_mutex()

free_packet_map(old_packets)  // Outside lock
```

#### Step 1.3: Create Worker-Local Scratch Space
**File**: `src/mesh/core/mesh.h` (new structure)

```c
typedef struct {
    chunk_mesh** mesh_array;
    int capacity;
    int count;
    chunk_mesh_map temp_map;  // Build here before swapping
} chunk_mesh_builder;

chunk_mesh_builder* builder_init(int initial_capacity);
void builder_add_mesh(chunk_mesh_builder* b, chunk_mesh* mesh);
chunk_mesh_map builder_finalize(chunk_mesh_builder* b);
void builder_cleanup(chunk_mesh_builder* b);
```

**Benefit**: Eliminate malloc/realloc during critical sections. Pre-allocated buffers reused per frame.

---

### Phase 2: World Mesh Worker Optimization
**Goal**: Reduce world mesh rebuild lock contention from seconds → milliseconds

#### Step 2.1: Pre-Allocate World Mesh Buffers
**File**: `src/mesh/generation/world_mesh.c`

```c
// Add to world mesh state:
typedef struct {
    int* transparent_data;
    int* opaque_data;
    int* liquid_data;
    int* foliage_data;
    float* custom_model_data;
    
    // Capacity tracking
    int transparent_capacity;
    int opaque_capacity;
    int liquid_capacity;
    int foliage_capacity;
    int custom_capacity;
} world_mesh_buffer;

// Global state:
world_mesh_buffer wm_buffer_active;     // Current render data
world_mesh_buffer wm_buffer_staging;    // Build here
pthread_mutex_t wm_swap_mutex;          // Atomic swap only
```

**Rationale**: Allocate large buffers once, reuse them. Avoid malloc inside locks.

#### Step 2.2: Single-Pass World Mesh Creation
**File**: `src/mesh/generation/world_mesh.c`

**Problem with current code**:
```c
// Pass 1: calculate total size (lock held)
for (int i = 0; i < count; i++) {
    total_transparent_sides += mesh->num_transparent_sides;
    // ... more accumulation
}
malloc(total_transparent_sides * ...);  // Under lock!

// Pass 2: copy data (lock held)
for (int i = 0; i < count; i++) {
    memcpy(...);  // Long operation under lock
}
```

**Solution - Single Pass with Pre-allocated Buffers**:
```c
// Before frame:
// - Buffers already allocated with max capacity
// - Just need to fill and track counts

// During update (no lock):
int transparent_offset = 0;
for (int i = 0; i < count; i++) {
    chunk_mesh* mesh = chunk_packets_buffer[i];
    if (mesh->num_transparent_sides > 0) {
        // Direct copy into pre-allocated buffer
        memcpy(wm_buffer_staging.transparent_data + transparent_offset,
               mesh->transparent_sides,
               mesh->num_transparent_sides * VBO_WIDTH * sizeof(int));
        transparent_offset += mesh->num_transparent_sides * VBO_WIDTH;
    }
}
wm_buffer_staging.num_transparent_sides = transparent_offset / VBO_WIDTH;

// Atomic swap (brief lock):
lock_wm_swap_mutex();
swap_buffers(&wm_buffer_active, &wm_buffer_staging);
broadcast_wm_update_signal();
unlock_wm_swap_mutex();
```

#### Step 2.3: Smart Buffer Resizing
Only reallocate when capacity exceeded (not every frame):

```c
void ensure_wm_buffer_capacity(world_mesh_buffer* buf, int transparent_needed,
                                int opaque_needed, int liquid_needed,
                                int foliage_needed, int custom_needed) {
    // Only allocate if exceeded current capacity
    if (transparent_needed > buf->transparent_capacity) {
        buf->transparent_data = realloc(buf->transparent_data, 
                                       transparent_needed * 1.5);  // 50% over-allocate
        buf->transparent_capacity = transparent_needed * 1.5;
    }
    // ... repeat for other buffers
}
```

---

### Phase 3: Sort Queue Optimization
**File**: `src/mesh/core/mesh.c`

#### Current Problem:
```c
lock_sort_queue_mutex()  // Long operation!
  sort_chunks_by_distance(sort_queue)
  // ... processing
unlock_sort_queue_mutex()
```

#### Solution: Local Sorting
```c
// 1. Snapshot queue briefly
lock_sort_queue_mutex()
temp_list = copy_queue_to_array(sort_queue)
queue_clear(sort_queue)
unlock_sort_queue_mutex()  // Release lock immediately

// 2. Sort locally
quicksort(temp_list, ...)

// 3. Reinsert if needed
lock_sort_queue_mutex()
queue_insert_many(sort_queue, temp_list)
unlock_sort_queue_mutex()
```

---

### Phase 4: Chunk Load Queue Optimization
**Similar pattern**: Copy, process locally, re-insert atomically

```c
// Snapshot
lock_chunk_load_queue_mutex()
pending_loads = copy_queue_to_array(chunk_load_queue)
queue_clear(chunk_load_queue)
unlock_chunk_load_queue_mutex()

// Process (no lock)
for (chunk_coord in pending_loads) {
    chunk* loaded = load_chunk_from_disk(coord)
    insert_into_world(loaded)
}

// Swap results if needed
lock_chunk_load_queue_mutex()
queue_insert_many(chunk_load_queue, results)
unlock_chunk_load_queue_mutex()
```

---

## Implementation Steps

### Step 1: Foundation (Session 1)
1. ✅ Add double-buffer state to `mesh.c`
2. Add `chunk_mesh_builder` structure and functions
3. Create `world_mesh_buffer` structure
4. Add condition variables for signaling

### Step 2: Chunk Mesh Worker (Session 2)
1. Refactor `get_chunk_meshes()` into builder pattern
2. Implement atomic packet swap with brief locking
3. Update `update_chunk_meshes()` to use new pattern
4. Test with varying render distances

### Step 3: World Mesh Worker (Session 3)
1. Convert `create_world_mesh()` to pre-allocated buffer approach
2. Implement single-pass filling
3. Add smart buffer resizing
4. Update `update_world_mesh()` with atomic swaps

### Step 4: Queue Optimizations (Session 4)
1. Extract "copy, process, swap" pattern into helper
2. Refactor sort queue operations
3. Refactor chunk load queue operations
4. Profile and measure lock contention

### Step 5: Testing & Tuning (Session 5)
1. Profile lock times before/after
2. Adjust buffer pre-allocation sizes
3. Test with high render distances (12-16)
4. Verify frame rate improvements

---

## Expected Performance Improvements

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Chunk mesh lock time | 5-50ms | <1ms | 5-50x |
| World mesh lock time | 10-100ms | <2ms | 5-50x |
| Main thread stall on lock | Frequent | Rare | High |
| Worker thread throughput | 1 chunk/frame | 8-16 chunks/frame | 8-16x |
| Frame time variance | High | Low | Stable |

---

## Key Benefits

1. **Reduced contention**: Workers spend 95% of time building data, 5% swapping
2. **Independent buffering**: Chunk worker independent from world mesh worker
3. **Scalable**: Works with N worker threads (pool ready)
4. **Cache friendly**: Locality preserved, fewer cache coherency bounces
5. **Predictable**: Lock times O(1), independent of data size
6. **Gradual**: Can implement phase-by-phase, test incrementally

---

## Code Patterns to Reuse

### Fast Snapshot Pattern:
```c
lock(mutex);
snapshot = copy_data();
clear_original();
unlock(mutex);
// Process snapshot safely
```

### Atomic Swap Pattern:
```c
lock(swap_mutex);
temp = *active;
*active = *staging;
*staging = temp;
signal(condition);
unlock(swap_mutex);
free_old_data(staging);  // Outside lock
```

### Pre-allocated Buffer Pattern:
```c
ensure_capacity(buffer, needed_size);  // Inside-frame, no lock
fill_buffer(buffer, data);              // No lock
lock_swap_mutex();
swap_buffers(&active, &staging);
unlock_swap_mutex();
```

---

## Potential Gotchas

1. **ABA problem**: Use generation numbers or epoch counters if pointers can cycle
2. **Reader races**: Brief window where reader might see partial data during swap - mitigate with read-copy-update or seqlock (advanced)
3. **Buffer exhaustion**: Pre-allocate conservatively (1.5x max observed)
4. **Stale data**: Render thread must handle brief stale mesh packets - should already do this

---

## Next Steps

1. Review this plan
2. Start with Phase 1 foundation work
3. Profile current implementation to establish baseline
4. Implement chunk mesh worker optimization first (highest impact)
5. Move to world mesh worker optimization
6. Measure improvements at each phase

