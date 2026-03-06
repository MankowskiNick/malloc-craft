#include "core.h"

#include "../world/generation/biome.h"
#include "../world/core/block.h"
#include "../world/core/world.h"
#include "settings.h"


void init_core(void) {
    read_settings("res/settings.json");

    init_biomes("res/biomes.json");
    init_blocks("res/blocks.json");
    init_world();
}

void core_cleanup(void) {
    world_cleanup();
    biome_cleanup();
}