#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

layout(set =1 , binding = 0) uniform texture2D my_tex; // texture unit
layout(set =1 , binding = 1) uniform sampler my_sampler; // The sampler object

vec2 CorrectUv(vec2 uv)
{
return vec2(uv.x, 1-uv.y);
}
vec2 xCorrectUv(vec2 uv)
{
return uv;
}

void main() {
    outColor = texture( sampler2D( my_tex, my_sampler ), texCoord );
}