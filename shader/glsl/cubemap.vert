#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 outWorldPosition;

layout(push_constant) uniform Push {
    mat4 projection;
    mat4 view;
} push;

void main()
{
    outWorldPosition = inPosition;
    gl_Position =  push.projection * push.view * vec4(outWorldPosition, 1.0);
}