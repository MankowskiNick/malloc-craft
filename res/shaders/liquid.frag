#version 330 core

in vec2 texCoord;
in vec2 atlasCoord;
in vec3 normal;
in vec3 fragPos;
in float dist;
flat in int faceType;

out vec4 FragColor;

uniform sampler2D atlas;
uniform sampler2D bump;
uniform sampler2D shadowMap;
uniform sampler2D caustic;

uniform float atlasSize;
uniform float time;
uniform float shadowSoftness;
uniform float shadowBias;
uniform int shadowSamples;
uniform float fogDistance;
uniform float waterLevel;

uniform mat4 sunView;
uniform mat4 sunProj;
uniform vec3 sunPos;
uniform vec3 sunColor;
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
    
    if (sunPos.y < -0.2) {
        return vec3(0.0);
    }
    
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

void main() {
    vec2 coord = (atlasCoord + texCoord) / atlasSize;
    
    vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz * 0.3);
    
    vec3 sun = normalize(sunPos);
    
    float diffuse = getDiffuse(bumpedNormal, sun);
    
    vec3 viewDir = normalize(cameraPos - fragPos);
    
    float spec = getSpecular(bumpedNormal, sun, viewDir, sunSpecularStrength, waterShininess);
    
    vec3 shadowFactor = getShadowIntensity(texCoord);
    vec3 lightIntensity = ambientLight + (shadowFactor * sunColor * diffuse);
    
    vec4 baseColor;
    
    // Apply chromatic aberration and caustics to side (0-3) and bottom (5) faces
    if (faceType != 4) {  // NOT the top face
        vec4 causticColor = getCausticColor(texCoord);
        vec4 textureColor = texture(atlas, coord);
        baseColor = vec4(textureColor.rgb * lightIntensity, textureColor.a);
        // Blend caustic color with the base texture
        baseColor.rgb = mix(baseColor.rgb, causticColor.rgb + textureColor.rgb, 0.3);
    } else {
        baseColor = texture(atlas, coord);
        baseColor.rgb *= lightIntensity;
    }
    
    vec3 finalColor = baseColor.rgb;
    finalColor += shadowFactor * sunColor * spec;
    
    vec4 colorWithFog = mix(vec4(finalColor, baseColor.a),
        vec4(1.0, 1.0, 1.0, 1.0),
        clamp(dist / fogDistance, 0.0, 1.0));

    FragColor = colorWithFog;
}
