#version 330 core

in vec3 fragPos; // position of the fragment
in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float dist; // distance from the camera
flat in float underwater;
flat in float y;
in vec3 normal;

out vec4 FragColor;

// textures
uniform float atlasSize;
uniform sampler2D atlas;
uniform sampler2D bump;
uniform sampler2D caustic;
uniform sampler2D shadowMap;

// shadows
uniform float shadowSoftness;
uniform float shadowBias;
uniform int shadowSamples;

// fog
uniform float fogDistance;

// water values
uniform float waterDistance;
uniform float waterLevel;
uniform float waterOffset;
uniform float time;

// sun
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform mat4 sunView;
uniform mat4 sunProj;

// ambient lighting
uniform vec3 ambientLight;


vec4 calculateChromaticAberration(vec2 coord) {
    vec4 causticColor = texture(caustic, coord);
    causticColor.r = texture(caustic, vec2(coord.x + 0.01, coord.y)).r;
    causticColor.b = texture(caustic, vec2(coord.x - 0.01, coord.y)).b;
    causticColor.g = texture(caustic, vec2(coord.x, coord.y + 0.01)).g;
    return causticColor;
}

vec4 getCausticColor(vec2 coord) {
    vec2 causticCoord = vec2(coord.x + sin(time * 0.08), coord.y + cos(time * 0.08));

    return calculateChromaticAberration(causticCoord);
}

vec4 getUnderwaterColor(vec2 coord, vec3 lightIntensity) {
    vec4 causticColor = getCausticColor(texCoord);
    vec4 textureColor = texture(atlas, coord);
    vec4 baseColor = vec4(textureColor.rgb * lightIntensity, textureColor.a);
    return mix(
            causticColor + texture(atlas, coord), 
            vec4(0.0, 0.0, 1.0, 1.0), 
            clamp(dist / waterDistance, 0.3, 0.8));
}

vec4 getStandardColor(vec2 coord, vec3 lightIntensity) {
    vec4 textureColor = texture(atlas, coord);
    vec4 baseColor = vec4(textureColor.rgb * lightIntensity, textureColor.a);
    return mix(baseColor, 
        vec4(1.0, 1.0, 1.0, 1.0), 
        clamp(dist / fogDistance, 0.0, 1.0));
}

vec3 getShadowIntensity(vec2 coord) {
    vec3 fragLightPos = vec3(sunProj * sunView * vec4(fragPos, 1.0));

    vec2 uvCoord;
    uvCoord.x = 0.5 * fragLightPos.x + 0.5;
    uvCoord.y = 0.5 * fragLightPos.y + 0.5;

    float z = 0.5 * fragLightPos.z + 0.5;
    
    // total shadow if night time
    if (sunPos.y < -0.2) {
        return vec3(0.0);
    }
    
    // PCF (Percentage-Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    int sampleCount = 0;
    for(int x = -(shadowSamples / 2); x <= (shadowSamples / 2); ++x) {
        for(int y = -(shadowSamples / 2); y <= (shadowSamples / 2); ++y) {
            vec2 sampleCoord = uvCoord + vec2(x, y) * texelSize * shadowSoftness;
            float depth = texture(shadowMap, sampleCoord).r;
            shadow += (depth < z - shadowBias) ? 0.0 : 1.0;
            sampleCount++;
        }
    }
    shadow /= float(sampleCount);
    
    return vec3(shadow);
}

void main() {
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    // get bumped normal
    vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz);
    // vec3 bumpedNormal = normal;

    // normalize sun position(also direction, since this is global)
    vec3 sun = normalize(sunPos);

    // calculate ambient and diffuse lighting
    float intensity = max(dot(bumpedNormal, sun), 0.0) * sunIntensity;
    vec3 shadowFactor = getShadowIntensity(texCoord);
    vec3 lightIntensity = ambientLight + (sunColor * intensity * shadowFactor);
    
    // underwater blocks are handled differently than standard blocks
    // underwater blocks are colored by the caustic texture and are bluer
    if (underwater == 1.0 && y <= waterLevel + 1.0 - waterOffset) {
        FragColor = getUnderwaterColor(coord, lightIntensity);
    }

    // standard blocks 
    else {
        FragColor = getStandardColor(coord, lightIntensity);
    }
}