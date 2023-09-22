#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec2 pos;

layout (set = 0, binding =0) uniform Camera
{
    mat4 Matrix;
}projection;

layout (set= 1, binding =0) uniform Model
{
    mat4 Matrix;
}ubo;

void main() {
    gl_Position = projection.Matrix * ubo.Matrix * vec4(position.xyz, 1.0);
    pos = position.xy;
}