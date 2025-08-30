#include <player_instance.h>
#include <asset.h>

camera parse_camera(json_object cam_obj) {
    camera cam;

    json_object position = json_get_property(cam_obj, "position");
    json_object up = json_get_property(cam_obj, "up");
    json_object front = json_get_property(cam_obj, "front");
    json_object yaw = json_get_property(cam_obj, "yaw");
    json_object pitch = json_get_property(cam_obj, "pitch");

    if (position.type != JSON_LIST || position.value.list.count != 3) {
        fprintf(stderr, "Error: camera position is not a list of 3 floats in player.json\n");
        exit(EXIT_FAILURE);
    }
    if (up.type != JSON_LIST || up.value.list.count != 3) {
        fprintf(stderr, "Error: camera up is not a list of 3 floats in player.json\n");
        exit(EXIT_FAILURE);
    }
    if (front.type != JSON_LIST || front.value.list.count != 3) {
        fprintf(stderr, "Error: camera front is not a list of 3 floats in player.json\n");
        exit(EXIT_FAILURE);
    }
    if (yaw.type != JSON_NUMBER) {
        fprintf(stderr, "Error: camera yaw is not a number in player.json\n");
        exit(EXIT_FAILURE);
    }
    if (pitch.type != JSON_NUMBER) {
        fprintf(stderr, "Error: camera pitch is not a number in player.json\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; i++) {
        if (position.value.list.items[i].type != JSON_NUMBER ||
            up.value.list.items[i].type != JSON_NUMBER ||
            front.value.list.items[i].type != JSON_NUMBER) {
            fprintf(stderr, "Error: camera vector component is not a number in player.json\n");
            exit(EXIT_FAILURE);
        }
        cam.position[i] = position.value.list.items[i].value.number;
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
        hotbar[i] = strdup(item.value.string);
    }

    return hotbar;
}

player_instance player_init(char* player_file) {

    char* player_json = read_file_to_string(player_file);
    json obj = deserialize_json(player_json, strlen(player_json));

    json_object hotbar_obj = json_get_property(obj.root, "hotbar");
    json_object cam_obj = json_get_property(obj.root, "cam");

    char** hotbar = parse_hotbar(hotbar_obj);
    camera cam = parse_camera(cam_obj);

    player_instance player = {
        .hotbar = hotbar,
        .hotbar_size = hotbar_obj.value.list.count,
        .cam = cam,
        .selected_block = 0
    };
    player.selected_block = 0;

    return player;
}
