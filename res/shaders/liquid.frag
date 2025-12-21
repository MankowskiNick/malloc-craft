
#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
flat in float underwater; // 1 if the fragment is underwater, 0 otherwise
flat in float y;
in vec3 normal;
in vec3 fragPos;
in float dist; // distance from the camera

out vec4 FragColor;

// texture info
uniform sampler2D atlas;
uniform sampler2D bump;
uniform float atlasSize;
uniform sampler2D shadowMap;
uniform sampler2D reflectionMap;

// shadows
uniform float shadowSoftness;
uniform float shadowBias;
uniform int shadowSamples;

// fog
uniform float fogDistance;

// water values
uniform float waterLevel;
uniform float waterOffset;

// sun
uniform mat4 sunView;
uniform mat4 sunProj;
uniform vec3 sunPos;
uniform vec3 sunColor;
uniform float sunIntensity;
uniform float sunSpecularStrength;
uniform float waterShininess;

uniform vec3 ambientLight;

uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 proj;

float getDiffuse(vec3 normal, vec3 lightDir) {
    return max(dot(normal, lightDir), 0.0);
}

float getSpecular(vec3 normal, vec3 lightDir, vec3 viewDir, float strength, float shininess) {
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    return strength * spec;
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

    // get the texture coordinate
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    // get bumped normal
    vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz);
    // vec3 bumpedNormal = normal;

    // normalize sun position(also direction, since this is global)
    vec3 sun = normalize(sunPos);

    // diffuse lighting
    float diffuse = getDiffuse(bumpedNormal, sun);

    // specular lighting
    vec3 viewDir = normalize(cameraPos - fragPos);

    float spec = getSpecular(bumpedNormal, sun, viewDir, sunSpecularStrength, waterShininess);

    // light intensity
    vec3 shadowFactor = getShadowIntensity(texCoord);
    vec3 lightIntensity = ambientLight + (shadowFactor * sunColor * diffuse);
    
    if (y == waterLevel + 1.0) {
        // Calculate reflection coordinates
        vec4 reflectionCoord = proj * view * vec4(fragPos, 1.0);
        vec2 reflectionUV = (reflectionCoord.xy / reflectionCoord.w) * 0.5 + 0.5;
        
        // Flip Y coordinate to make reflection appear upside down
        reflectionUV.y = 1.0 - reflectionUV.y;
        
        // Add wave distortion to reflection coordinates
        vec2 distortion = (texture(bump, coord).xy - 0.5) * 0.02;
        reflectionUV += distortion;
        
        // Clamp reflection coordinates to valid range
        reflectionUV = clamp(reflectionUV, 0.0, 1.0);
        
        // Sample reflection texture
        vec3 reflectionColor = texture(reflectionMap, reflectionUV).rgb;
        
        // Calculate fresnel effect (view-dependent reflection intensity)
        float fresnel = pow(1.0 - max(dot(viewDir, bumpedNormal), 0.0), 2.0);
        
        vec4 baseColor = texture(atlas, coord);
        
        // Blend reflection with base water color
        vec3 finalColor = mix(baseColor.rgb, reflectionColor, fresnel * 0.4);
        
        // Apply lighting
        finalColor = finalColor * lightIntensity;
        
        // Add specular highlights on top
        finalColor += shadowFactor * sunColor * spec;
        
        // Apply fog effect - mix with white fog color based on distance
        vec4 colorWithFog = mix(vec4(finalColor, baseColor.a), 
            vec4(1.0, 1.0, 1.0, 1.0), 
            clamp(dist / fogDistance, 0.0, 1.0));
        
        FragColor = colorWithFog;
    }
    else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
