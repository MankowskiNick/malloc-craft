#include <settings.h>
#include <cerialize/cerialize.h>
#include <asset.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int WIDTH = 1200;
int HEIGHT = 900;
char* TITLE = "malloc-craft";
// 0.0001047 represents 2π / (60000 ms), giving a full day cycle in ~60 seconds
float TIME_SCALE = 0.0001047f;
int CHUNK_LOAD_PER_FRAME = 1;
int CHUNK_CACHE_SIZE = 1024;
int WIREFRAME = 0;
char* ATLAS_PATH = "res/textures/atlas.png";
char* BUMP_PATH = "res/textures/bump.png";
char* SKYBOX_PATH = "res/textures/skybox.png";
char* CAUSTIC_PATH = "res/textures/caustic.png";
int ATLAS_TEXTURE_INDEX = 0;
int BUMP_TEXTURE_INDEX = 1;
int CAUSTIC_TEXTURE_INDEX = 2;
int SKYBOX_TEXTURE_INDEX = 3;
int SHADOW_MAP_TEXTURE_INDEX = 4;
int REFLECTION_MAP_TEXTURE_INDEX = 5;
int VSYNC = 1;
int CHUNK_RENDER_DISTANCE = 16;
int SHADOW_MAP_WIDTH = 10000;
int SHADOW_MAP_HEIGHT = 10000;
float SHADOW_RENDER_DIST = 16.0f * 16.0f;
float SHADOW_SOFTNESS = 3.0f;
int SHADOW_SAMPLES = 4;
float SHADOW_BIAS = 0.0025f;
int REFLECTION_FBO_WIDTH = 512;
int REFRACTION_FBO_HEIGHT = 512;
int TICK_RATE = 32;
float FOV = 60.0f;
float RENDER_DISTANCE = 500.0f;
int ATLAS_SIZE = 32;
int USE_MIPMAP = 1;
int SKYBOX_SLICES = 15;
int SKYBOX_STACKS = 15;
float SKYBOX_RADIUS = 1.0f;
int SUN_SLICES = 15;
int SUN_STACKS = 15;
float SUN_RADIUS = 0.1f;
float SUN_INTENSITY = 1.0f;
float SUN_SPECULAR_STRENGTH = 10.0f;
float WATER_SHININESS = 5000.0f;
float AMBIENT_R_INTENSITY = 0.3f;
float AMBIENT_G_INTENSITY = 0.3f;
float AMBIENT_B_INTENSITY = 0.3f;
float WATER_OFFSET = 0.2f;
float WATER_HEIGHT = 1.0f;
float WATER_DISTANCE = 50.0f;
float DELTA_X = 0.1f;
float DELTA_Y = 0.1f;
float DELTA_Z = 0.1f;
float SENSITIVITY = 0.001f;
float MAX_REACH = 5.0f;
float RAY_STEP = 0.05f;
int SEED = 42069;
float WORLDGEN_BIOME_FREQUENCY = 0.2f;
float WORLDGEN_BIOME_AMPLITUDE = 1.0f;
int WORLDGEN_BIOME_OCTAVES = 2;
float WORLDGEN_BLOCKHEIGHT_MODIFIER = 40.0f;
int WORLDGEN_BASE_TERRAIN_HEIGHT = 64; // The height of the base terrain
int WORLDGEN_WATER_LEVEL = 64; // The height of the water level
float WORLDGEN_BLOCKHEIGHT_FREQUENCY = 0.25f;
float WORLDGEN_BLOCKHEIGHT_AMPLITUDE = 2.0f;
int WORLDGEN_BLOCKHEIGHT_OCTAVES = 6;
char* BLOCK_FILE = "res/blocks.json";
char* PLAYER_FILE = "res/player.json";

void parse_display_settings(json_object display_obj) {
    if (display_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: display section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object width = json_get_property(display_obj, "width");
    if (width.type == JSON_NUMBER) {
        WIDTH = (int)width.value.number;
    }

    json_object height = json_get_property(display_obj, "height");
    if (height.type == JSON_NUMBER) {
        HEIGHT = (int)height.value.number;
    }

    json_object title = json_get_property(display_obj, "title");
    if (title.type == JSON_STRING) {
        TITLE = strdup(title.value.string);
    }

    json_object vsync = json_get_property(display_obj, "vsync");
    if (vsync.type == JSON_BOOL) {
        VSYNC = vsync.value.boolean ? 1 : 0;
    }

    json_object fov = json_get_property(display_obj, "fov");
    if (fov.type == JSON_NUMBER) {
        FOV = fov.value.number;
    }
}

void parse_chunks_settings(json_object chunks_obj) {
    if (chunks_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: chunks section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object chunk_load_per_frame = json_get_property(chunks_obj, "chunk_load_per_frame");
    if (chunk_load_per_frame.type == JSON_NUMBER) {
        CHUNK_LOAD_PER_FRAME = (int)chunk_load_per_frame.value.number;
    }

    json_object chunk_cache_size = json_get_property(chunks_obj, "chunk_cache_size");
    if (chunk_cache_size.type == JSON_NUMBER) {
        CHUNK_CACHE_SIZE = (int)chunk_cache_size.value.number;
    }

    json_object chunk_render_distance = json_get_property(chunks_obj, "chunk_render_distance");
    if (chunk_render_distance.type == JSON_NUMBER) {
        CHUNK_RENDER_DISTANCE = (int)chunk_render_distance.value.number;
    }
}

void parse_graphics_settings(json_object graphics_obj) {
    if (graphics_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: graphics section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object wireframe = json_get_property(graphics_obj, "wireframe");
    if (wireframe.type == JSON_BOOL) {
        WIREFRAME = wireframe.value.boolean ? 1 : 0;
    }

    json_object render_distance = json_get_property(graphics_obj, "render_distance");
    if (render_distance.type == JSON_NUMBER) {
        RENDER_DISTANCE = render_distance.value.number;
    }

    json_object atlas_size = json_get_property(graphics_obj, "atlas_size");
    if (atlas_size.type == JSON_NUMBER) {
        ATLAS_SIZE = (int)atlas_size.value.number;
    }

    json_object use_mipmap = json_get_property(graphics_obj, "use_mipmap");
    if (use_mipmap.type == JSON_BOOL) {
        USE_MIPMAP = use_mipmap.value.boolean ? 1 : 0;
    }

    json_object tick_rate = json_get_property(graphics_obj, "tick_rate");
    if (tick_rate.type == JSON_NUMBER) {
        TICK_RATE = (int)tick_rate.value.number;
    }
}

void parse_textures_settings(json_object textures_obj) {
    if (textures_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: textures section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object atlas_path = json_get_property(textures_obj, "atlas_path");
    if (atlas_path.type == JSON_STRING) {
        ATLAS_PATH = strdup(atlas_path.value.string);
    }

    json_object bump_path = json_get_property(textures_obj, "bump_path");
    if (bump_path.type == JSON_STRING) {
        BUMP_PATH = strdup(bump_path.value.string);
    }

    json_object skybox_path = json_get_property(textures_obj, "skybox_path");
    if (skybox_path.type == JSON_STRING) {
        SKYBOX_PATH = strdup(skybox_path.value.string);
    }

    json_object caustic_path = json_get_property(textures_obj, "caustic_path");
    if (caustic_path.type == JSON_STRING) {
        CAUSTIC_PATH = strdup(caustic_path.value.string);
    }

    json_object atlas_texture_index = json_get_property(textures_obj, "atlas_texture_index");
    if (atlas_texture_index.type == JSON_NUMBER) {
        ATLAS_TEXTURE_INDEX = (int)atlas_texture_index.value.number;
    }

    json_object bump_texture_index = json_get_property(textures_obj, "bump_texture_index");
    if (bump_texture_index.type == JSON_NUMBER) {
        BUMP_TEXTURE_INDEX = (int)bump_texture_index.value.number;
    }

    json_object caustic_texture_index = json_get_property(textures_obj, "caustic_texture_index");
    if (caustic_texture_index.type == JSON_NUMBER) {
        CAUSTIC_TEXTURE_INDEX = (int)caustic_texture_index.value.number;
    }

    json_object skybox_texture_index = json_get_property(textures_obj, "skybox_texture_index");
    if (skybox_texture_index.type == JSON_NUMBER) {
        SKYBOX_TEXTURE_INDEX = (int)skybox_texture_index.value.number;
    }

    json_object shadow_map_texture_index = json_get_property(textures_obj, "shadow_map_texture_index");
    if (shadow_map_texture_index.type == JSON_NUMBER) {
        SHADOW_MAP_TEXTURE_INDEX = (int)shadow_map_texture_index.value.number;
    }
}

void parse_shadows_settings(json_object shadows_obj) {
    if (shadows_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: shadows section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object shadow_map_width = json_get_property(shadows_obj, "shadow_map_width");
    if (shadow_map_width.type == JSON_NUMBER) {
        SHADOW_MAP_WIDTH = (int)shadow_map_width.value.number;
    }

    json_object shadow_map_height = json_get_property(shadows_obj, "shadow_map_height");
    if (shadow_map_height.type == JSON_NUMBER) {
        SHADOW_MAP_HEIGHT = (int)shadow_map_height.value.number;
    }

    json_object shadow_render_dist = json_get_property(shadows_obj, "shadow_render_dist");
    if (shadow_render_dist.type == JSON_NUMBER) {
        SHADOW_RENDER_DIST = shadow_render_dist.value.number;
    }

    json_object shadow_softness = json_get_property(shadows_obj, "shadow_softness");
    if (shadow_softness.type == JSON_NUMBER) {
        SHADOW_SOFTNESS = shadow_softness.value.number;
    }

    json_object shadow_samples = json_get_property(shadows_obj, "shadow_samples");
    if (shadow_samples.type == JSON_NUMBER) {
        SHADOW_SAMPLES = (int)shadow_samples.value.number;
    }

    json_object shadow_bias = json_get_property(shadows_obj, "shadow_bias");
    if (shadow_bias.type == JSON_NUMBER) {
        SHADOW_BIAS = shadow_bias.value.number;
    }
}

void parse_reflections_settings(json_object reflections_obj) {
    if (reflections_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: reflections section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object reflection_fbo_width = json_get_property(reflections_obj, "reflection_fbo_width");
    if (reflection_fbo_width.type == JSON_NUMBER) {
        REFLECTION_FBO_WIDTH = (int)reflection_fbo_width.value.number;
    }

    json_object refraction_fbo_height = json_get_property(reflections_obj, "refraction_fbo_height");
    if (refraction_fbo_height.type == JSON_NUMBER) {
        REFRACTION_FBO_HEIGHT = (int)refraction_fbo_height.value.number;
    }
}

void parse_skybox_settings(json_object skybox_obj) {
    if (skybox_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: skybox section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object skybox_slices = json_get_property(skybox_obj, "skybox_slices");
    if (skybox_slices.type == JSON_NUMBER) {
        SKYBOX_SLICES = (int)skybox_slices.value.number;
    }

    json_object skybox_stacks = json_get_property(skybox_obj, "skybox_stacks");
    if (skybox_stacks.type == JSON_NUMBER) {
        SKYBOX_STACKS = (int)skybox_stacks.value.number;
    }

    json_object skybox_radius = json_get_property(skybox_obj, "skybox_radius");
    if (skybox_radius.type == JSON_NUMBER) {
        SKYBOX_RADIUS = skybox_radius.value.number;
    }
}

void parse_sun_settings(json_object sun_obj) {
    if (sun_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: sun section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object sun_slices = json_get_property(sun_obj, "sun_slices");
    if (sun_slices.type == JSON_NUMBER) {
        SUN_SLICES = (int)sun_slices.value.number;
    }

    json_object sun_stacks = json_get_property(sun_obj, "sun_stacks");
    if (sun_stacks.type == JSON_NUMBER) {
        SUN_STACKS = (int)sun_stacks.value.number;
    }

    json_object sun_radius = json_get_property(sun_obj, "sun_radius");
    if (sun_radius.type == JSON_NUMBER) {
        SUN_RADIUS = sun_radius.value.number;
    }

    json_object sun_intensity = json_get_property(sun_obj, "sun_intensity");
    if (sun_intensity.type == JSON_NUMBER) {
        SUN_INTENSITY = sun_intensity.value.number;
    }

    json_object sun_specular_strength = json_get_property(sun_obj, "sun_specular_strength");
    if (sun_specular_strength.type == JSON_NUMBER) {
        SUN_SPECULAR_STRENGTH = sun_specular_strength.value.number;
    }
}

void parse_lighting_settings(json_object lighting_obj) {
    if (lighting_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: lighting section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object ambient_r_intensity = json_get_property(lighting_obj, "ambient_r_intensity");
    if (ambient_r_intensity.type == JSON_NUMBER) {
        AMBIENT_R_INTENSITY = ambient_r_intensity.value.number;
    }

    json_object ambient_g_intensity = json_get_property(lighting_obj, "ambient_g_intensity");
    if (ambient_g_intensity.type == JSON_NUMBER) {
        AMBIENT_G_INTENSITY = ambient_g_intensity.value.number;
    }

    json_object ambient_b_intensity = json_get_property(lighting_obj, "ambient_b_intensity");
    if (ambient_b_intensity.type == JSON_NUMBER) {
        AMBIENT_B_INTENSITY = ambient_b_intensity.value.number;
    }
}

void parse_water_settings(json_object water_obj) {
    if (water_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: water section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object water_offset = json_get_property(water_obj, "water_offset");
    if (water_offset.type == JSON_NUMBER) {
        WATER_OFFSET = water_offset.value.number;
    }

    json_object water_height = json_get_property(water_obj, "water_height");
    if (water_height.type == JSON_NUMBER) {
        WATER_HEIGHT = water_height.value.number;
    }

    json_object water_distance = json_get_property(water_obj, "water_distance");
    if (water_distance.type == JSON_NUMBER) {
        WATER_DISTANCE = water_distance.value.number;
    }

    json_object water_shininess = json_get_property(water_obj, "water_shininess");
    if (water_shininess.type == JSON_NUMBER) {
        WATER_SHININESS = water_shininess.value.number;
    }
}

void parse_player_settings(json_object player_obj) {
    if (player_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: player section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object delta_x = json_get_property(player_obj, "delta_x");
    if (delta_x.type == JSON_NUMBER) {
        DELTA_X = delta_x.value.number;
    }

    json_object delta_y = json_get_property(player_obj, "delta_y");
    if (delta_y.type == JSON_NUMBER) {
        DELTA_Y = delta_y.value.number;
    }

    json_object delta_z = json_get_property(player_obj, "delta_z");
    if (delta_z.type == JSON_NUMBER) {
        DELTA_Z = delta_z.value.number;
    }

    json_object sensitivity = json_get_property(player_obj, "sensitivity");
    if (sensitivity.type == JSON_NUMBER) {
        SENSITIVITY = sensitivity.value.number;
    }

    json_object max_reach = json_get_property(player_obj, "max_reach");
    if (max_reach.type == JSON_NUMBER) {
        MAX_REACH = max_reach.value.number;
    }

    json_object ray_step = json_get_property(player_obj, "ray_step");
    if (ray_step.type == JSON_NUMBER) {
        RAY_STEP = ray_step.value.number;
    }
}

void parse_world_generation_settings(json_object worldgen_obj) {
    if (worldgen_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: world_generation section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object seed = json_get_property(worldgen_obj, "seed");
    if (seed.type == JSON_NUMBER) {
        SEED = (int)seed.value.number;
    }

    json_object biome_generation = json_get_property(worldgen_obj, "biome_generation");
    if (biome_generation.type == JSON_OBJECT) {
        json_object frequency = json_get_property(biome_generation, "frequency");
        if (frequency.type == JSON_NUMBER) {
            WORLDGEN_BIOME_FREQUENCY = frequency.value.number;
        }

        json_object amplitude = json_get_property(biome_generation, "amplitude");
        if (amplitude.type == JSON_NUMBER) {
            WORLDGEN_BIOME_AMPLITUDE = amplitude.value.number;
        }

        json_object octaves = json_get_property(biome_generation, "octaves");
        if (octaves.type == JSON_NUMBER) {
            WORLDGEN_BIOME_OCTAVES = (int)octaves.value.number;
        }
    }

    json_object terrain = json_get_property(worldgen_obj, "terrain");
    if (terrain.type == JSON_OBJECT) {
        json_object blockheight_modifier = json_get_property(terrain, "blockheight_modifier");
        if (blockheight_modifier.type == JSON_NUMBER) {
            WORLDGEN_BLOCKHEIGHT_MODIFIER = blockheight_modifier.value.number;
        }

        json_object base_terrain_height = json_get_property(terrain, "base_terrain_height");
        if (base_terrain_height.type == JSON_NUMBER) {
            WORLDGEN_BASE_TERRAIN_HEIGHT = (int)base_terrain_height.value.number;
        }

        json_object water_level = json_get_property(terrain, "water_level");
        if (water_level.type == JSON_NUMBER) {
            WORLDGEN_WATER_LEVEL = (int)water_level.value.number;
        }

        json_object blockheight_frequency = json_get_property(terrain, "blockheight_frequency");
        if (blockheight_frequency.type == JSON_NUMBER) {
            WORLDGEN_BLOCKHEIGHT_FREQUENCY = blockheight_frequency.value.number;
        }

        json_object blockheight_amplitude = json_get_property(terrain, "blockheight_amplitude");
        if (blockheight_amplitude.type == JSON_NUMBER) {
            WORLDGEN_BLOCKHEIGHT_AMPLITUDE = blockheight_amplitude.value.number;
        }

        json_object blockheight_octaves = json_get_property(terrain, "blockheight_octaves");
        if (blockheight_octaves.type == JSON_NUMBER) {
            WORLDGEN_BLOCKHEIGHT_OCTAVES = (int)blockheight_octaves.value.number;
        }
    }
}

void parse_game_data_settings(json_object gamedata_obj) {
    if (gamedata_obj.type != JSON_OBJECT) {
        fprintf(stderr, "Error: game_data section is not an object in settings.json\n");
        exit(EXIT_FAILURE);
    }

    json_object tree_count = json_get_property(gamedata_obj, "tree_count");
    if (tree_count.type == JSON_NUMBER) {
        TREE_COUNT = (int)tree_count.value.number;
    }

    json_object block_file = json_get_property(gamedata_obj, "block_file");
    if (block_file.type == JSON_STRING) {
        BLOCK_FILE = strdup(block_file.value.string);
    }

    json_object player_file = json_get_property(gamedata_obj, "player_file");
    if (player_file.type == JSON_STRING) {
        PLAYER_FILE = strdup(player_file.value.string);
    }

    json_object time_scale = json_get_property(gamedata_obj, "time_scale");
    if (time_scale.type == JSON_NUMBER) {
        TIME_SCALE = time_scale.value.number;
    }
}

void read_settings(const char* filename) {
    // Read the JSON file to string
    char* settings_json = read_file_to_string(filename);
    if (settings_json == NULL) {
        fprintf(stderr, "Error: Failed to read settings file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Parse the JSON
    json obj = deserialize_json(settings_json, strlen(settings_json));
    if (obj.failure) {
        fprintf(stderr, "Error: Failed to parse JSON in settings file '%s': %s\n", filename, obj.error_text);
        free(settings_json);
        exit(EXIT_FAILURE);
    }

    // Validate that root is an object
    if (obj.root.type != JSON_OBJECT) {
        fprintf(stderr, "Error: Root of settings.json must be an object\n");
        free(settings_json);
        json_free(&obj);
        exit(EXIT_FAILURE);
    }

    // Parse each settings section
    json_object display_obj = json_get_property(obj.root, "display");
    if (display_obj.type != JSON_NULL) {
        parse_display_settings(display_obj);
    }

    json_object chunks_obj = json_get_property(obj.root, "chunks");
    if (chunks_obj.type != JSON_NULL) {
        parse_chunks_settings(chunks_obj);
    }

    json_object graphics_obj = json_get_property(obj.root, "graphics");
    if (graphics_obj.type != JSON_NULL) {
        parse_graphics_settings(graphics_obj);
    }

    json_object textures_obj = json_get_property(obj.root, "textures");
    if (textures_obj.type != JSON_NULL) {
        parse_textures_settings(textures_obj);
    }

    json_object shadows_obj = json_get_property(obj.root, "shadows");
    if (shadows_obj.type != JSON_NULL) {
        parse_shadows_settings(shadows_obj);
    }

    json_object skybox_obj = json_get_property(obj.root, "skybox");
    if (skybox_obj.type != JSON_NULL) {
        parse_skybox_settings(skybox_obj);
    }

    json_object sun_obj = json_get_property(obj.root, "sun");
    if (sun_obj.type != JSON_NULL) {
        parse_sun_settings(sun_obj);
    }

    json_object lighting_obj = json_get_property(obj.root, "lighting");
    if (lighting_obj.type != JSON_NULL) {
        parse_lighting_settings(lighting_obj);
    }

    json_object water_obj = json_get_property(obj.root, "water");
    if (water_obj.type != JSON_NULL) {
        parse_water_settings(water_obj);
    }

    json_object player_obj = json_get_property(obj.root, "player");
    if (player_obj.type != JSON_NULL) {
        parse_player_settings(player_obj);
    }

    json_object worldgen_obj = json_get_property(obj.root, "world_generation");
    if (worldgen_obj.type != JSON_NULL) {
        parse_world_generation_settings(worldgen_obj);
    }

    json_object gamedata_obj = json_get_property(obj.root, "game_data");
    if (gamedata_obj.type != JSON_NULL) {
        parse_game_data_settings(gamedata_obj);
    }

    // Clean up memory
    free(settings_json);
    json_free(&obj);
}
