#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;
layout (location = 5) in int aOrientation;

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
    if(face == 1) { // west face (+X)
        return vec3(1.0, pos.y, pos.x);
    } else if(face == 3) { // east face (-X)
        return vec3(0.0, pos.y, pos.x);
    } else if(face == 0) { // north face (-Z)
        return vec3(pos.x, pos.y, 0.0);
    } else if(face == 2) { // south face (+Z)
        return vec3(pos.x, pos.y, 1.0);
    } else if(face == 4) { // up face (+Y)
        return vec3(pos.x, 1.0, pos.y);
    } else if(face == 5) { // down face (-Y)
        return vec3(pos.x, 0.0, pos.y);
    }
    return pos;
}

vec2 transformUV(vec2 uv, int side, int orientation) {
    switch(orientation) {
        case 0:
            if (side == 1 || side == 3) {
                return vec2(uv.y, uv.x);
            }
            return uv;
        case 1:
            if (side == 0 || side == 2 || side == 4 || side == 5) {
                return vec2(1.0 - uv.y, 1.0 - uv.x);
            }
            return uv;
        case 2:
            if (side == 1 || side == 3) {
                return vec2(uv.y, 1.0 - uv.x);
            }
            return vec2(1.0 - uv.x, 1.0 - uv.y);
        case 3:
            if (side == 0 || side == 2 || side == 4 || side == 5) {
                return vec2(uv.y, uv.x);
            }
            return uv;
        case 4:
            if (side < 4) {
                return vec2(uv.x, 1.0 - uv.y);
            }
        default:
            return uv;
    }
}

vec3 getNormal(vec3 pos, int face) {
    if (face == 1) {
        return vec3(0.0, 0.0, 1.0);
    } else if (face == 3) {
        return vec3(0.0, 0.0, -1.0);
    } else if (face == 0) {
        return vec3(-1.0, 0.0, 0.0);
    } else if (face == 2) {
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

    texCoord = transformUV(aPos.xy, aSide, aOrientation);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    // offset water level for waves
    y = worldPos.y;

    dist = length(gl_Position.xyz);
    underwater = float(aUnderwater);
    normal = getNormal(aPos, aSide);

    fragPos = worldPos;
}