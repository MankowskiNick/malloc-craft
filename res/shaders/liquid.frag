
#version 330 core

in vec2 texCoord; // coordinate of the quad [0, 1]
in vec2 atlasCoord; // coordinate of the texture in the atlas [0, 32]
in float underwater; // 1 if the fragment is underwater, 0 otherwise
in float y;
in vec3 normal;
in vec3 fragPos;

out vec4 FragColor;

// texture info
uniform sampler2D atlas;
uniform sampler2D bump;
uniform float atlasSize;
uniform sampler2D shadowMap;

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
    float depth = texture(shadowMap, uvCoord).r;

    float bias = 0.0025;

    if (depth <= z - bias) {
        return vec3(0.0, 0.0, 0.0);
    }
    else {
        return vec3(1.0, 1.0, 1.0);
    }
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
    vec3 lightIntensity = ambientLight + (shadowFactor * sunColor * diffuse) + (shadowFactor * sunColor * spec);
    
    if (y == waterLevel + 1.0) {
        vec4 baseColor = texture(atlas, coord);
        FragColor = vec4(baseColor.rgb * lightIntensity, baseColor.a);
    }
    else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}