#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;
layout (location = 5) in int aOrientation;
layout (location = 6) in int aWaterLevel;
layout (location = 7) in int aWaterLevelTransition;
layout (location = 8) in int aLodScale;
layout (location = 9) in int aAO;

out vec2 texCoord;
out vec2 atlasCoord;
out float dist;
out vec3 normal;
out vec3 fragPos;
out float aoFactor;

uniform mat4 view;
uniform mat4 proj;
uniform float time;

vec3 transformFace(vec3 pos, int face) {
    vec3 side = pos;
    if(face == 1) { // west face (+X)
        side = vec3(1.0, pos.y, pos.x);
    } else if(face == 3) { // east face (-X)
        side = vec3(0.0, pos.y, pos.x);
    } else if(face == 0) { // north face (-Z)
        side = vec3(pos.x, pos.y, 0.0);
    } else if(face == 2) { // south face (+Z)
        side = vec3(pos.x, pos.y, 1.0);
    } else if(face == 4) { // up face (+Y)
        side = vec3(pos.x, 1.0, pos.y);
    } else if(face == 5) { // down face (-Y)
        side = vec3(pos.x, 0.0, pos.y);
    }
    side = float(aLodScale) * side;
    return side;
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

// AO constants
const int AO_MAX = 3;
const int AO_BITS_PER_VERTEX = 2;
const int AO_MASK = 3;  // 0b11 - mask for 2 bits
const float AO_MIN_LIGHT = 0.4;
const float AO_CURVE = 1.0 - AO_MIN_LIGHT;

// Unpack AO value for the current vertex based on position in quad
// aPos.xy represents position in quad: (0,0), (1,0), (1,1), (0,1)
float getVertexAO(int packedAO, vec2 pos) {
    // Determine vertex index from quad position
    // Vertices: v0(0,0), v1(1,0), v2(1,1), v3(0,1)
    int vertexIndex = int(pos.x + 0.5) + int(pos.y + 0.5) * 2;
    // Remap: (0,0)->0, (1,0)->1, (0,1)->2, (1,1)->3
    // But we want: (0,0)->0, (1,0)->1, (1,1)->2, (0,1)->3
    if (vertexIndex == 2) vertexIndex = 3;
    else if (vertexIndex == 3) vertexIndex = 2;

    // Extract 2-bit AO value for this vertex
    int ao = (packedAO >> (vertexIndex * AO_BITS_PER_VERTEX)) & AO_MASK;

    // Convert to lighting factor: 0 -> AO_MIN_LIGHT (dark), 3 -> 1.0 (full bright)
    return AO_MIN_LIGHT + AO_CURVE * (float(ao) / float(AO_MAX));
}

void main() {
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    vec4 viewPos = view * vec4(worldPos, 1.0);
    gl_Position = proj * viewPos;

    texCoord = transformUV(aPos.xy, aSide, aOrientation);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    dist = length(viewPos.xyz);
    normal = getNormal(aPos, aSide);
    fragPos = worldPos;

    // Calculate AO factor for this vertex
    aoFactor = getVertexAO(aAO, aPos.xy);
}