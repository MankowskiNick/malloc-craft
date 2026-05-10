#version 330 core 

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;
layout (location = 8) in int aLodScale;

uniform mat4 sunView;
uniform mat4 sunProj;

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

    return float(aLodScale) * side;
}

void main() {
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    gl_Position = sunProj * sunView * vec4(worldPos, 1.0);
}
