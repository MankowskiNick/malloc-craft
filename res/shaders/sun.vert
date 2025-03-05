#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 sunPos;

void main() {
    // this is hacky, but it works.  i am struggling to find out why the sun is 90 degrees off from all lighting effects.
    // i think it has to do with the fact that OpenGL is left handed in screen space, but right handed in world space.
    // i'm not sure though.
    vec3 rotatedPos = vec3(sunPos.z, sunPos.y, -sunPos.x); 
    gl_Position = proj * view * vec4(position + rotatedPos, 1.0);
}