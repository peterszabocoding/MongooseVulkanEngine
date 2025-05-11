#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main()
{
    outTexCoord = inTexCoord;
    gl_Position = vec4(inPosition, 1.0);
}
