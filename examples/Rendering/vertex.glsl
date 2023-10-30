#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in int materialId;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out flat int outMaterialId;

layout(set = 0, binding = 0) uniform Camera 
{
    mat4 ProjectionView;
}Cam;


void main() {
    gl_Position = Cam.ProjectionView * vec4(position.xyz, 1.0);

    outTexCoord = texCoord;
    outMaterialId = materialId;
}