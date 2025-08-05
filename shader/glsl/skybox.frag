#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform samplerCube textures[];

layout(push_constant) uniform Push {
    uint skyboxTextureIndex;
} push;

void main()
{
    vec3 envColor = texture(textures[push.skyboxTextureIndex], WorldPos).rgb;

    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    FragColor = vec4(envColor, 1.0);
}
