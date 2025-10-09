#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec2 texCoord;
out vec3 normal;
out vec3 fragPos;
out float dist;

uniform mat4 view;
uniform mat4 proj;
uniform float atlasSize;

void main() {
    gl_Position = proj * view * vec4(aPos, 1.0);
    
    texCoord = vec2(aUV.x, aUV.y);
    
    normal = aNormal;
    fragPos = aPos;
    
    dist = length(gl_Position.xyz);
}
