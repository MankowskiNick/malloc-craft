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

    // Treat level 6 as source block (same height as 7) for seamless rendering
    float effectiveWaterLevel = float(aWaterLevel);
    float effectiveTransitionLevel = float(aWaterLevelTransition);
    
    if (aWaterLevel == 6) {
        effectiveWaterLevel = 7.0;
    }
    if (aWaterLevelTransition == 6) {
        effectiveTransitionLevel = 7.0;
    }

    // Handle water transitions (when water level changes between adjacent blocks)
    // aWaterLevelTransition != 0 means this is a transition face
    if (aWaterLevelTransition > 0) {
        // This is a transition face connecting two water blocks at different levels
        // aWaterLevel = higher water level (current block)
        // aWaterLevelTransition = lower water level (adjacent block)
        
        // For cardinal directions (sides 0-3), create a sloped connector
        if (aSide >= 0 && aSide <= 3) {
            // Map aPos.y (0-1) to the transition height range
            float lowerHeight = (7.0 - effectiveTransitionLevel) / 7.0;
            float upperHeight = (7.0 - effectiveWaterLevel) / 7.0;
            
            // Interpolate between lower and upper heights based on texture Y coordinate
            float interpolatedHeight = mix(lowerHeight, upperHeight, aPos.y);
            
            // Adjust the Y position of the transition face
            // The face extends from lowerHeight at aPos.y=0 to upperHeight at aPos.y=1
            worldPos.y = instancePos.y + 1.0 - interpolatedHeight;
        }
    }
    else {
        // Normal water faces (not transitions)
        
        // For top face, adjust Y position based on water level
        if (aSide == 4) {
            // Water level 0-7, where 7 is full block (1.0)
            // Reduce Y by the inverse of water level
            float waterHeightReduction = (7.0 - effectiveWaterLevel) / 7.0;
            worldPos.y -= waterHeightReduction;
        }
        // For side faces (0-3), reduce the top edge Y position
        else if (aSide >= 0 && aSide <= 3) {
            // Only adjust the top vertices (where aPos.y == 1.0)
            if (aPos.y > 0.5) {
                float waterHeightReduction = (7.0 - effectiveWaterLevel) / 7.0;
                worldPos.y -= waterHeightReduction;
            }
        }
    }

    faceType = aSide;

    gl_Position = proj * view * vec4(worldPos, 1.0);

    texCoord = vec2(aPos.x, aPos.y);
    atlasCoord = vec2(aAtlasCoord.x, aAtlasCoord.y);

    // Get proper normal for this face
    normal = getFaceNormal(aSide);

    // For transition faces, compute a sloped normal
    if (aWaterLevelTransition > 0 && aSide >= 0 && aSide <= 3) {
        // Create a normal that faces both outward and slightly upward to reflect the slope
        vec3 outwardNormal = getFaceNormal(aSide);
        vec3 upwardNormal = vec3(0.0, 1.0, 0.0);
        
        // Compute slope vector using effective levels
        float heightDiff = (7.0 - effectiveTransitionLevel) / 7.0 - (7.0 - effectiveWaterLevel) / 7.0;
        float slopeStrength = abs(heightDiff);
        
        // Blend outward normal with upward component based on slope
        normal = normalize(mix(outwardNormal, upwardNormal, 0.3 * slopeStrength));
    }
    // For top face, add slight wave perturbation to normal
    else if (aSide == 4) {
        vec3 dx = vec3(1.0, 0.075 * 0.5, 0.0);
        vec3 dz = vec3(0.0, 0.075 * 0.5, 1.0);
        normal = normalize(cross(dz, dx));
    }

    fragPos = worldPos;

    dist = length(gl_Position.xyz);
}
