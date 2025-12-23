#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in ivec3 aInstancePos;
layout (location = 2) in ivec2 aAtlasCoord;
layout (location = 3) in int aSide;
layout (location = 4) in int aUnderwater;
layout (location = 5) in int aOrientation;
layout (location = 6) in int aWaterLevel;

out vec2 texCoord;
out vec2 atlasCoord;
out vec3 normal;
out vec3 fragPos;
out float dist;
flat out int faceType;

uniform mat4 view;
uniform mat4 proj;
uniform float waterLevel;
uniform float time;

vec3 transformFace(vec3 pos, int face) {
    vec3 result = vec3(0.0);
    if(face == 1) { // west face (+X)
        result = vec3(1.0, pos.y, pos.x);
    } else if(face == 3) { // east face (-X)
        result = vec3(0.0, pos.y, pos.x);
    } else if(face == 0) { // north face (-Z)
        result = vec3(pos.x, pos.y, 0.0);
    } else if(face == 2) { // south face (+Z)
        result = vec3(pos.x, pos.y, 1.0);
    } else if(face == 4) { // up face (+Y)
        result = vec3(pos.x, 1.0, pos.y);
    } else if(face == 5) { // down face (-Y)
        result = vec3(pos.x, 0.0, pos.y);
    }

    return result;
}

// Get normal vector for each face type
vec3 getFaceNormal(int face) {
    if (face == 0) return vec3(0.0, 0.0, -1.0);  // north (-Z)
    if (face == 1) return vec3(1.0, 0.0, 0.0);   // west (+X)
    if (face == 2) return vec3(0.0, 0.0, 1.0);   // south (+Z)
    if (face == 3) return vec3(-1.0, 0.0, 0.0);  // east (-X)
    if (face == 4) return vec3(0.0, 1.0, 0.0);   // up (+Y)
    if (face == 5) return vec3(0.0, -1.0, 0.0);  // down (-Y)
    return vec3(0.0, 1.0, 0.0);
}

void main()
{
    vec3 instancePos = vec3(aInstancePos);
    vec3 worldPos = transformFace(aPos, aSide) + instancePos;

    // For top face, adjust Y position based on water level
    if (aSide == 4) {
        // Water level 0-7, where 7 is full block (1.0)
        // Reduce Y by the inverse of water level
        float waterHeightReduction = (8.0 - float(aWaterLevel)) / 8.0;
        worldPos.y -= waterHeightReduction;
    }
    // For side faces (0-3), reduce the top edge Y position
    else if (aSide >= 0 && aSide <= 3) {
        // Only adjust the top vertices (where aPos.y == 1.0)
        if (aPos.y > 0.5) {
            float waterHeightReduction = (8.0 - float(aWaterLevel)) / 8.0;
            worldPos.y -= waterHeightReduction;
        }
    }

    faceType = aSide;

    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    // Get proper normal for this face
    normal = getFaceNormal(aSide);

    // For top face, add slight wave perturbation to normal
    if (aSide == 4) {
        vec3 dx = vec3(1.0, 0.075 * 0.5, 0.0);
        vec3 dz = vec3(0.0, 0.075 * 0.5, 1.0);
        normal = normalize(cross(dz, dx));
    }

    fragPos = worldPos;

    dist = length(gl_Position.xyz);
}
