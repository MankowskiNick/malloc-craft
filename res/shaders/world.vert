#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;

out vec2 texCoord;
out float y;
out vec2 atlasCoord;
out float dist;
out float underwater;
out vec3 normal;
out vec3 fragPos;

uniform mat4 view;
uniform mat4 proj;
uniform float time;

vec3 transformFace(vec3 pos, int face) {
    if(face == 0) { // front face (+Z)
        return vec3(pos.x, pos.y, 1.0);
    } else if(face == 1) { // back face (-Z)
        return vec3(pos.x, pos.y, 0.0);
    } else if(face == 2) { // left face (-X)
        return vec3(0.0, pos.y, pos.x);
    } else if(face == 3) { // right face (+X)
        return vec3(1.0, pos.y, pos.x);
    } else if(face == 4) { // top face (+Y)
        return vec3(pos.x, 1.0, pos.y);
    } else if(face == 5) { // bottom face (-Y)
        return vec3(pos.x, 0.0, pos.y);
    }
    return pos;
}

vec3 getNormal(vec3 pos, int face) {
    if (face == 0) {
        return vec3(0.0, 0.0, 1.0);
    } else if (face == 1) {
        return vec3(0.0, 0.0, -1.0);
    } else if (face == 2) {
        return vec3(-1.0, 0.0, 0.0);
    } else if (face == 3) {
        return vec3(1.0, 0.0, 0.0);
    } else if (face == 4) {
        return vec3(0.0, 1.0, 0.0);
    } else if (face == 5) {
        return vec3(0.0, -1.0, 0.0);
    }
    return vec3(0.0, 0.0, 0.0);
}

void main() {
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    // offset water level for waves
    y = worldPos.y;
    // y -= sin(worldPos.x * 0.5 + time) * 0.075;
    // y -= sin(worldPos.z * 0.5 + time) * 0.075;

    dist = length(gl_Position.xyz);
    underwater = float(aUnderwater);
    normal = getNormal(aPos, aSide);

    fragPos = worldPos;
}