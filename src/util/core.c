#include "core.h"

#include "../world/generation/biome.h"
#include "../world/core/block.h"
#include "../world/core/world.h"
#include "settings.h"

static char* get_settings_file(char env[16]) {
    char* file = malloc(64 * sizeof(char));
    if (env != NULL && strcmp(env, "") != 0) {
        strcpy(file, "res/settings.");
        strcat(file, env);
        strcat(file, ".json");
    } else {
        strcpy(file, "res/settings.json");
    }
    return file;
}

void load_core_settings(char env[16]) {
    char* settings_file = get_settings_file(env);
    read_settings(settings_file);
    free(settings_file);
}

void init_core(void) {
    init_biomes("res/biomes.json");
    init_blocks("res/blocks.json");
    init_world();
}

void core_cleanup(void) {
    world_cleanup();
    biome_cleanup();
}
