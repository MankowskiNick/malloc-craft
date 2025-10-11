#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;

uniform mat4 reflectionView;
uniform mat4 reflectionProj;
uniform float waterLevel;

out float gl_ClipDistance[1];
out vec2 TexCoord;
out vec2 AtlasCoord;

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

void main() {
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    // Clip geometry below water level for realistic reflections
    gl_ClipDistance[0] = worldPos.y - waterLevel;

    // Pass texture coordinates to fragment shader
    TexCoord = aPos.xy;
    AtlasCoord = vec2(aAtlasCoord);

    gl_Position = reflectionProj * reflectionView * vec4(worldPos, 1.0);
}
