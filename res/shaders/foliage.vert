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
    // Cross pattern: two diagonal planes intersecting
    if(face == 0) { 
        // First diagonal plane (NW to SE)
        // Map the quad's X into Z so the plane runs from (0,0) -> (1,1) in XZ
        // This produces a diagonal plane with z = x across the block (0..1),
        // so the intersection is centered at 0.5,0.5
        return vec3(pos.x, pos.y, pos.x);
    } else if(face == 1) { 
        // Second diagonal plane (NE to SW) 
        // Map the quad's X into Z mirrored so the plane runs from (0,1) -> (1,0)
        // i.e. z = 1 - x across the block (0..1)
        return vec3(pos.x, pos.y, 1.0 - pos.x);
    }
    // Default fallback (shouldn't be used for foliage)
    return pos;
}

vec3 getNormal(vec3 pos, int face) {
    if (face == 0) {
        // Normal for first diagonal plane (z = x) -> gradient (-1, 0, 1)
        return normalize(vec3(-0.707107, 0.0, 0.707107));
    } else if (face == 1) {
        // Normal for second diagonal plane (z = 1 - x) -> gradient (1, 0, 1)
        return normalize(vec3(0.707107, 0.0, 0.707107));
    }
    return vec3(0.0, 0.0, 0.0);
}

void main() {
    if (aSide > 1) {
        // Invalid face for foliage, discard
        gl_Position = vec4(0.0);
        return;
    }

    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    // Apply slight wave animation to foliage
    y = worldPos.y;
    y += sin(worldPos.x * 2.0 + time * 2.0) * 0.02;
    y += sin(worldPos.z * 2.0 + time * 2.0) * 0.02;

    dist = length(gl_Position.xyz);
    underwater = float(aUnderwater);
    normal = getNormal(aPos, aSide);

    fragPos = worldPos;
}
