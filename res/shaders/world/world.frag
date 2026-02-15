#version 330 core

in vec3 fragPos; // position of the fragment
in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float dist; // distance from the camera
in vec3 normal;
in float aoFactor; // ambient occlusion factor (0.4 - 1.0)

out vec4 FragColor;

uniform float atlasSize;
uniform sampler2D atlas;
uniform sampler2D bump;
uniform sampler2D shadowMap;
uniform float shadowSoftness;
uniform float shadowBias;
uniform int shadowSamples;
uniform float fogDistance;
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform mat4 sunView;
uniform mat4 sunProj;
uniform vec3 ambientLight;


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

    vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz);
    vec3 sun = normalize(sunPos);

    float intensity = max(dot(bumpedNormal, sun), 0.0) * sunIntensity;
    vec3 shadowFactor = getShadowIntensity(texCoord);
    vec3 lightIntensity = ambientLight + (sunColor * intensity * shadowFactor);

    // Apply ambient occlusion
    lightIntensity *= aoFactor;

    vec4 textureColor = texture(atlas, coord);
    vec4 baseColor = vec4(textureColor.rgb * lightIntensity, textureColor.a);
    FragColor = mix(baseColor,
        vec4(1.0, 1.0, 1.0, 1.0),
        clamp(dist / fogDistance, 0.0, 1.0));
}