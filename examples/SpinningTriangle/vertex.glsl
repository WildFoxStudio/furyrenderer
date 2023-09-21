#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragColor;

layout (binding =0) uniform MyMatrix
{
    mat4 Matrix;
}ubo;

void main() {
    gl_Position = ubo.Matrix * vec4(position.xyz, 1.0);
    fragColor = color;
}