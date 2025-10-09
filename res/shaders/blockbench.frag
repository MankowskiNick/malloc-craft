#version 330 core

in vec3 fragPos;
in vec2 texCoord;
in vec2 atlasCoord;
in float dist;
in vec3 normal;

out vec4 FragColor;

// textures
uniform float atlasSize;
uniform sampler2D atlas;
uniform sampler2D bump;
uniform sampler2D shadowMap;

// shadows
uniform float shadowSoftness;
uniform float shadowBias;
uniform int shadowSamples;

// fog
uniform float fogDistance;

// sun
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform mat4 sunView;
uniform mat4 sunProj;

// ambient lighting
uniform vec3 ambientLight;

vec3 getShadowIntensity() {
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
    // Use direct UV coordinates for blockbench models
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    // get bumped normal
    vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz);

    // normalize sun position (also direction, since this is global)
    vec3 sun = normalize(sunPos);

    // calculate ambient and diffuse lighting
    float intensity = max(dot(bumpedNormal, sun), 0.0) * sunIntensity;
    vec3 shadowFactor = getShadowIntensity();
    vec3 lightIntensity = ambientLight + (sunColor * intensity * shadowFactor);
    
    // Sample texture and apply lighting
    vec4 textureColor = texture(atlas, coord);
    vec4 baseColor = vec4(textureColor.rgb * lightIntensity, textureColor.a);
    
    // Apply fog
    FragColor = mix(baseColor, 
        vec4(1.0, 1.0, 1.0, 1.0), 
        clamp(dist / fogDistance, 0.0, 1.0));
}
