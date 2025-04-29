#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 outWorldPosition;

layout(binding = 0) uniform ProjectionViewBuffer {
    mat4 projection;
    mat4 view;
} pvb;

void main()
{
    outWorldPosition = inPosition;
    gl_Position =  pvb.projection * pvb.view * vec4(outWorldPosition, 1.0);
}