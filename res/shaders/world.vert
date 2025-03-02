#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;

out vec2 texCoord;
out vec2 atlasCoord;
out float dist;

uniform mat4 view;
uniform mat4 proj;

vec3 transformFace(vec3 pos, int face) {
    if(face == 0) { // front face (+X)
        return vec3(1.0, pos.y, pos.x);
    } else if(face == 1) { // back face (-X)
        return vec3(0.0, pos.y, pos.x);
    } else if(face == 2) { // left face (-Z)
        return vec3(pos.x, pos.y, 0.0);
    } else if(face == 3) { // right face (+Z)
        return vec3(pos.x, pos.y, 1.0);
    } else if(face == 4) { // top face (+Y)
        return vec3(pos.x, 1.0, pos.y);
    } else if(face == 5) { // bottom face (-Y)
        return vec3(pos.x, 0.0, pos.y);
    }
    return pos;
}

void main()
{
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;
    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    dist = length(gl_Position.xyz);
}