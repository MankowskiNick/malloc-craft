
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

// fog
uniform float fogDistance;

// water values
uniform float waterLevel;
uniform float waterOffset;

// sun
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

void main() {

    // get the texture coordinate
    vec2 coord = (atlasCoord + texCoord) / atlasSize;

    // get bumped normal
    // vec3 bumpedNormal = normalize(normal + texture(bump, coord).xyz);
    vec3 bumpedNormal = normal;

    // normalize sun position(also direction, since this is global)
    vec3 sun = normalize(sunPos);

    // diffuse lighting
    float diffuse = getDiffuse(bumpedNormal, sun);

    // specular lighting
    vec3 viewDir = normalize(cameraPos - fragPos);
    float spec = getSpecular(bumpedNormal, sun, viewDir, sunSpecularStrength, waterShininess);

    // light intensity
    vec3 lightIntensity = ambientLight + (sunColor * diffuse) + (sunColor * spec);
    
    if (y == waterLevel + 1.0) {
        vec4 baseColor = texture(atlas, coord);
        FragColor = vec4(baseColor.rgb * lightIntensity, baseColor.a);
    }
    else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}