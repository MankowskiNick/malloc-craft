#include <biome.h>
#include <block_models.h>
#include <noise.h>
#include <settings.h>
#include <cerialize/cerialize.h>
#include <asset.h>


#include <stdio.h>

biome* BIOMES = NULL;
int BIOME_COUNT = 0;

void copy_biome_data(json_object obj) {
    int size = obj.value.list.count;
    BIOMES = malloc(sizeof(biome) * size);
    if (BIOMES == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for biomes\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < size; i++) {
        json_object biome = obj.value.list.items[i];
        if (biome.type != JSON_OBJECT) {
            fprintf(stderr, "Error: Biome entry is not an object\n");
            free(BIOMES);
            exit(EXIT_FAILURE);
        }

        json_object id_obj = json_get_property(biome, "id");
        json_object name_obj = json_get_property(biome, "name");
        json_object surface_type_obj = json_get_property(biome, "surface_type");
        json_object subsurface_type_obj = json_get_property(biome, "subsurface_type");
        json_object underground_type_obj = json_get_property(biome, "underground_type");
        json_object underwater_type_obj = json_get_property(biome, "underwater_type");
        json_object tree_type_obj = json_get_property(biome, "tree_type");
        json_object tree_density_obj = json_get_property(biome, "tree_density");

        if (id_obj.type != JSON_NUMBER 
            || name_obj.type != JSON_STRING 
            || surface_type_obj.type != JSON_STRING 
            || subsurface_type_obj.type != JSON_STRING
            || underground_type_obj.type != JSON_STRING 
            || underwater_type_obj.type != JSON_STRING
            || tree_type_obj.type != JSON_NUMBER 
            || tree_density_obj.type != JSON_NUMBER) {
            fprintf(stderr, "Error: Invalid biome data types\n");
            free(BIOMES);
            exit(EXIT_FAILURE);
        }

        BIOMES[i].id = (short)id_obj.value.number;
        BIOMES[i].name = strdup(name_obj.value.string);
        BIOMES[i].surface_type = surface_type_obj.value.string;
        BIOMES[i].subsurface_type = subsurface_type_obj.value.string;
        BIOMES[i].underground_type = underground_type_obj.value.string;
        BIOMES[i].underwater_type = underwater_type_obj.value.string;
        BIOMES[i].tree_type = (short)tree_type_obj.value.number;
        BIOMES[i].tree_density = tree_density_obj.value.number;
    }

    BIOME_COUNT = size;
}

void read_biomes(char* filename) {
    char* biomes_json = read_file_to_string(filename);
    
    if (biomes_json == NULL) {
        fprintf(stderr, "Error: Could not read biomes from %s\n", filename);
        exit(EXIT_FAILURE);
    }

    json obj = deserialize_json(biomes_json, strlen(biomes_json));
    if (obj.failure) {
        fprintf(stderr, "Error: Could not deserialize biomes from %s: %s\n", filename, obj.error_text);
        exit(EXIT_FAILURE);
    }

    free(biomes_json);

    if (obj.root.type != JSON_LIST) {
        fprintf(stderr, "Error: Biomes are not in a list format in %s\n", filename);
        json_free(&obj);
        exit(EXIT_FAILURE);
    }

    copy_biome_data(obj.root);
}

short get_biome_id(float x, float z) {
    float noise = n_get(x, z, 
        WORLDGEN_BIOME_FREQUENCY, 
        WORLDGEN_BIOME_AMPLITUDE, 
        WORLDGEN_BIOME_OCTAVES);
    float biome = noise * (float)BIOME_COUNT; // multiply by number of biomes
    return (uint)biome;
}

biome* get_biome(float x, float z) {
    short id = get_biome_id(x, z);
    return &BIOMES[id];
}