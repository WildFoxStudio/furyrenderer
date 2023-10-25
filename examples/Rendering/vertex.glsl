#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 texCoord;

layout(set = 0, binding = 0) uniform Camera 
{
    mat4 ProjectionView;
}Cam;

void main() {
    gl_Position = Cam.ProjectionView * vec4(position.xyz, 1.0);
    fragColor = color;

    texCoord = position.xy;
}