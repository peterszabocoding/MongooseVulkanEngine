#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(set = 2, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;

layout(location = 0) out vec3 WorldPos;

void main()
{
    WorldPos = inPosition;

    mat4 rotView = mat4(mat3(transforms.view));
    vec4 clipPos = transforms.projection * rotView * vec4(WorldPos, 1.0);

    gl_Position = clipPos.xyww;
}
