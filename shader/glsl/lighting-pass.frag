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

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec4 inWorldPosition;
layout(location = 5) in vec3 inViewPosition;
layout(location = 6) in mat3 TBN;

// ------------------------------------------------------------------
// OUTPUT VARIABLES -------------------------------------------------
// ------------------------------------------------------------------

layout(location = 0) out vec4 finalImage;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(push_constant) uniform Push {
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

// Transform uniforms
layout(set = 2, binding = 0) uniform CameraBuffer {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} camera;

// Light uniforms
layout(std430, set = 2, binding = 1) uniform Lights {
    mat4[SHADOW_MAP_CASCADE_COUNT] lightProjection;
    vec4 color;
    vec3 direction;
    float ambientIntensity;
    vec4[SHADOW_MAP_CASCADE_COUNT] cascadeSplits;
    float intensity;
    float bias;
} lights;

layout(set = 2, binding = 2)    uniform sampler2DArray shadowMap;

// Irradiance uniformsí
layout(set = 2, binding = 3)    uniform samplerCube irradianceMap;

// Post Processing
layout(set = 2, binding = 4)    uniform sampler2D SSAO;

// Reflection uniforms
layout(set = 2, binding = 5)    uniform samplerCube prefilterMap;
layout(set = 2, binding = 6)    uniform sampler2D brdfLUT;



// ------------------------------------------------------------------
// VARIABLES --------------------------------------------------------
// ------------------------------------------------------------------
vec3 N;
vec3 V;

// ------------------------------------------------------------------
// CONSTANTS --------------------------------------------------------
// ------------------------------------------------------------------

const float MAX_REFLECTION_LOD = 6.0;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0);

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
    normal.xy = normalFromTexture.rg;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normalize(TBN * normal);
}

vec3 CalcDirectionalLightRadiance(vec3 direction, vec4 shadowMapCoord, int cascadeIndex)
{
    vec3 lightDir = direction;
    float diffuseFactor = clamp(dot(N, lightDir), 0.0, 1.0);

    float shadowCoeff = filterPCFCascaded(shadowMap, shadowMapCoord / shadowMapCoord.w, cascadeIndex);
    return shadowCoeff * lights.intensity * lights.color.rgb * diffuseFactor;
}

void main() {
    vec3 baseColor;
    MaterialParamsObject material = materials.params[push.materialIndex];

    if (material.baseColorTextureIndex < INVALID_TEXTURE_INDEX)
    {
        vec4 baseColorSampled = texture(textures[material.baseColorTextureIndex], fragTexCoord);
        if (baseColorSampled.a < 0.5) discard;

        baseColor = pow(baseColorSampled.rgb, vec3(2.2));
    } else {
        if (material.baseColor.a < 0.5) discard;
        baseColor = material.baseColor.rgb;
    }

    int cascadeIndex = 0;
    for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if (inViewPosition.z < lights.cascadeSplits[i].x) {
            cascadeIndex = i + 1;
        }
    }

    vec4 shadowMapCoord = (biasMat * lights.lightProjection[cascadeIndex]) * inWorldPosition;

    vec3 albedo = fragColor * material.tint.rgb * baseColor;

    float metallic = material.metallic;
    float roughness = material.roughness;
    float occlusion = 1.0;

    if (material.metallicRoughnessTextureIndex < INVALID_TEXTURE_INDEX)
    {
        vec4 metallicRoughness = texture(textures[material.metallicRoughnessTextureIndex], fragTexCoord);

        occlusion =  metallicRoughness.r;
        metallic =  metallicRoughness.b;
        roughness = metallicRoughness.g;
    }

    vec3 normalMapColor = texture(textures[material.normalMapTextureIndex], fragTexCoord).rgb;

    N = material.normalMapTextureIndex < INVALID_TEXTURE_INDEX
    ? CalcSurfaceNormal(normalMapColor, TBN)
    : fragNormal;
    V = normalize(camera.cameraPosition - inWorldPosition.xyz);

    vec3 lightPosition = vec3(0.0);
    // vec3 L = normalize(lightPosition - inWorldPosition.xyz);
    vec3 L = -lights.direction;
    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04);
    F0      = mix(F0, albedo, metallic);

    //float distance    = length(lightPosition - inWorldPosition.xyz);
    //float attenuation = 1.0 / (distance * distance);
    //vec3 radiance     = lights.intensity * lights.color.rgb * attenuation;

    vec3 baseReflectivity = vec3(0.04);
    baseReflectivity = mix(baseReflectivity, albedo, metallic);

    float NdotV = max(dot(N, V), 0.000001);
    vec3 F = fresnelSchlickRoughness(NdotV, baseReflectivity, roughness);
    vec3 kD = (1.0 - F) * (1.0 - metallic);

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 R = reflect(-V, N);
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * (brdf.x + brdf.y));

    vec3 radiance = CalcDirectionalLightRadiance(-lights.direction, shadowMapCoord, cascadeIndex);
    vec3 Lo = CalcLightRadiance(L, H, V, N, F0, albedo, roughness, metallic, radiance);

    ivec2 texSize = textureSize(SSAO, 0);
    vec2 texCoord = ((gl_FragCoord.xy + vec2(1.0)) * 0.5) / texSize;
    float SSAOValue = texture(SSAO, texCoord).r;

    vec3 ambient = lights.ambientIntensity * (kD * diffuse + specular);

    vec3 color = (ambient + Lo) * SSAOValue;

    finalImage = vec4(color, 1.0);
}