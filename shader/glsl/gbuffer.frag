#version 450

#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

#include <shadow_mapping.glslh>
#include <pbr_functions.glslh>

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

// Material uniforms
layout(set = 0, binding = 0) uniform MaterialParams {
    vec4 tint;
    vec4 baseColor;

    float metallic;
    float roughness;

    bool useBaseColorMap;
    bool useNormalMap;
    bool useMetallicRoughnessMap;
    bool alphaTested;
} materialParams;

layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessSampler;

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
    normal.xy = normalFromTexture.rg;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normalize(TBN * normal);
}

void main() {
    vec4 baseColorSampled = texture(baseColorSampler, fragTexCoord);
    vec3 baseColor = materialParams.useBaseColorMap ? pow(baseColorSampled.rgb, vec3(2.2)) : materialParams.baseColor.rgb;

    vec3 normalMapColor = texture(normalSampler, fragTexCoord).rgb;
    vec3 N = materialParams.useNormalMap ? CalcSurfaceNormal(normalMapColor, TBN) : fragNormal;

    /////////////////   GBuffer   ////////////////////////
    normalImage = vec4(N, 1.0);
    outWorldPosition = vec4(inWorldPosition.xyz, 1.0);
    /////////////////////////////////////////////////////
}
