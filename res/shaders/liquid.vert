#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;

out vec2 texCoord;
out vec2 atlasCoord;
out float underwater;
out float y;
out vec3 normal;
out vec3 fragPos;

uniform mat4 view;
uniform mat4 proj;
uniform float waterOffset;
uniform float waterLevel;
uniform float time;

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

    // offset water level for waves
    y = worldPos.y;
    if (worldPos.y == waterLevel + 1.0) {
        worldPos.y -= waterOffset;
    //     worldPos.y += 0.075 * sin(0.5 * (worldPos.x + worldPos.z) + time);
    }
    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    underwater = float(aUnderwater);

    // calculate normal vector
    vec3 dx = vec3(
        1.0,
        0.075 * 0.5,// * cos(0.5 * (worldPos.x + worldPos.z) + time),
        0.0
    );

    vec3 dz = vec3(
        0.0,
        0.075 * 0.5,// * cos(0.5 * (worldPos.x + worldPos.z) + time),
        1.0
    );
    
    normal = normalize(cross(dz, dx));
    fragPos = worldPos;
}