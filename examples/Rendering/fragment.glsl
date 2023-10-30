#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec2 outTexCoord;
layout(location = 1) in flat int outMaterialId;
layout(location = 0) out vec4 outColor;

layout(set =1 , binding = 0) uniform texture2D my_tex[]; // texture unit
layout(set =1 , binding = 1) uniform sampler my_sampler[]; // The sampler object

layout(set = 1, binding = 2) uniform Material
{
    int MaterialId;
}Materials[];


vec2 CorrectUv(vec2 uv)
{
return vec2(uv.x, 1-uv.y);
}
vec2 xCorrectUv(vec2 uv)
{
return uv;
}

void main() {
    int materialIndex = Materials[outMaterialId].MaterialId;
    outColor = texture( sampler2D( my_tex[materialIndex], my_sampler[materialIndex] ), outTexCoord );
}