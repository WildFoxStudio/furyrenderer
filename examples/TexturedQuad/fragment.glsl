#version 450

layout(location = 0) in vec2 pos;
layout(location = 0) out vec4 outColor;

layout(set =1, binding = 0) uniform sampler2D MyTexture;

vec2 CorrectUv(vec2 uv)
{
return vec2(uv.x, 1-uv.y);
}
vec2 xCorrectUv(vec2 uv)
{
return uv;
}

void main() {
    vec4 texColor = texture(MyTexture, pos);
    outColor = texColor;
}