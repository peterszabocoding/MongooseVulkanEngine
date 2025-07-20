#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require
#extension GL_EXT_nonuniform_qualifier : require


#include <shadow_mapping.glslh>
#include <pbr_functions.glslh>

#define INVALID_TEXTURE_INDEX 65535

// ------------------------------------------------------------------
// INPUT VARIABLES --------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 inWorldPosition;
layout(location = 4) in mat3 TBN;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0)            out vec4 normalImage;
layout(location = 1)            out vec4 outWorldPosition;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 modelMatrix;
    uint materialIndex;
} push;

layout(set = 0, binding = 0) uniform sampler2D textures[];


struct MaterialParamsObject {
    vec4 tint;
    vec4 baseColor;

    float metallic;
    float roughness;

    uint baseColorTextureIndex;
    uint normalMapTextureIndex;
    uint metallicRoughnessTextureIndex;

    bool alphaTested;
};

layout(std140,set = 1, binding = 0) readonly buffer MaterialBuffer{
    MaterialParamsObject params[];
} materials;

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
    normal.xy = normalFromTexture.rg;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normalize(TBN * normal);
}

void main() {
    MaterialParamsObject material = materials.params[push.materialIndex];

    vec4 baseColorSampled = texture(textures[material.baseColorTextureIndex], fragTexCoord);
    vec3 baseColor = material.baseColorTextureIndex < INVALID_TEXTURE_INDEX ? pow(baseColorSampled.rgb, vec3(2.2)) : material.baseColor.rgb;

    vec3 normalMapColor = texture(textures[material.normalMapTextureIndex], fragTexCoord).rgb;
    vec3 N = material.normalMapTextureIndex < INVALID_TEXTURE_INDEX ? CalcSurfaceNormal(normalMapColor, TBN) : fragNormal;

    /////////////////   GBuffer   ////////////////////////
    normalImage = vec4(N, 1.0);
    outWorldPosition = vec4(inWorldPosition.xyz, 1.0);
    /////////////////////////////////////////////////////
}
