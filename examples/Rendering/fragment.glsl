#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;


vec2 CorrectUv(vec2 uv)
{
return vec2(uv.x, 1-uv.y);
}
vec2 xCorrectUv(vec2 uv)
{
return uv;
}

void main() {
    outColor = fragColor;
}