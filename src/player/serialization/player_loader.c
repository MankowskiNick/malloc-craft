#include "player_loader.h"
#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

camera parse_camera(json_object cam_obj) {
    camera cam;

    json_object position = json_get_property(cam_obj, "position");
    json_object up = json_get_property(cam_obj, "up");
    json_object front = json_get_property(cam_obj, "front");
    json_object yaw = json_get_property(cam_obj, "yaw");
    json_object pitch = json_get_property(cam_obj, "pitch");

    assert(up.type == JSON_LIST && up.value.list.count == 3 && "Player JSON \"up\" must be a list of 3 floats.");
    assert(front.type == JSON_LIST && front.value.list.count == 3 && "Player JSON \"front\" must be a list of 3 floats.");
    assert(yaw.type == JSON_NUMBER && "Player JSON \"yaw\" must be a number.");
    assert(pitch.type == JSON_NUMBER && "Player JSON \"pitch\" must be a number.");

    for (int i = 0; i < 3; i++) {

        assert(up.value.list.items[i].type == JSON_NUMBER
            && front.value.list.items[i].type == JSON_NUMBER
            && "Error: camera vector component is not a number in player.json\n");

        cam.up[i] = up.value.list.items[i].value.number;
        cam.front[i] = front.value.list.items[i].value.number;
    }

    cam.yaw = yaw.value.number;
    cam.pitch = pitch.value.number;

    return cam;
}

char** parse_hotbar(json_object hotbar_obj) {
    if (hotbar_obj.type != JSON_LIST) {
        fprintf(stderr, "Error: hotbar is not a list in player.json\n");
        exit(EXIT_FAILURE);
    }

    if (hotbar_obj.value.list.count > 10 || hotbar_obj.value.list.count < 1) {
        fprintf(stderr, "Error: hotbar must contain between 1 and 10 items in `player.json`\n");
        exit(EXIT_FAILURE);
    }

    char** hotbar = malloc(hotbar_obj.value.list.count * sizeof(char*));
    for (int i = 0; i < hotbar_obj.value.list.count; i++) {
        json_object item = hotbar_obj.value.list.items[i];
        if (item.type != JSON_STRING) {
            fprintf(stderr, "Error: hotbar item %d is not a string in player.json\n", i);
            exit(EXIT_FAILURE);
        }
        if (item.value.string == NULL || item.value.string[0] == '\0') {
            fprintf(stderr, "Error: hotbar item %d has invalid or empty string in player.json\n", i);
            exit(EXIT_FAILURE);
        }
        hotbar[i] = strdup(item.value.string);
        if (hotbar[i] == NULL) {
            fprintf(stderr, "Error: failed to allocate memory for hotbar item %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return hotbar;
}

float parse_height(json_object height_obj) {
    assert(height_obj.type == JSON_NUMBER && "Player JSON \"height\" object must be a number.");
    return height_obj.value.number;
}

float parse_radius(json_object radius_obj) {
    assert(radius_obj.type == JSON_NUMBER && "Player JSON \"radius\" object must be a number.");
    return radius_obj.value.number;
}

float* parse_position(json_object position_object) {
    assert(position_object.type == JSON_LIST 
        && position_object.value.list.count == 3 
        && "Player JSON \"position\" must be a list of length 3.");

    for (int i = 0; i < 3; i++) {
        json_object obj = position_object.value.list.items[i];
        assert(obj.type == JSON_NUMBER && "Player JSON \"position\" must only contain floats/ints.");
    }

    float* pos = (float*)malloc(3 * sizeof(float));
    pos[0] = position_object.value.list.items[0].value.number;    
    pos[1] = position_object.value.list.items[1].value.number;
    pos[2] = position_object.value.list.items[2].value.number;

    return pos;
}
