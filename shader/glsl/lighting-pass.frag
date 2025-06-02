#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

#include <shadow_mapping.glslh>
#include <pbr_functions.glslh>

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

// Transform uniforms
layout(set = 1, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;

// Light uniforms
layout(std430, set = 2, binding = 0) uniform Lights {
    mat4[SHADOW_MAP_CASCADE_COUNT] lightProjection;
    vec4 color;
    vec3 direction;
    float ambientIntensity;
    float[SHADOW_MAP_CASCADE_COUNT] cascadeSplits;
    float intensity;
    float bias;
} lights;
layout(set = 2, binding = 1)    uniform sampler2DArray shadowMap;

// Irradiance uniforms
layout(set = 3, binding = 0)    uniform samplerCube irradianceMap;

// Post Processing
layout(set = 4, binding = 0)    uniform sampler2D SSAO;

// Reflection uniforms
layout(set = 5, binding = 0)    uniform samplerCube prefilterMap;
layout(set = 5, binding = 1)    uniform sampler2D brdfLUT;

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
0.5, 0.5, 0.0, 1.0 );

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
    if (materialParams.useBaseColorMap)
    {
        vec4 baseColorSampled = texture(baseColorSampler, fragTexCoord);
        if (baseColorSampled.a < 0.5) discard;

        baseColor = pow(baseColorSampled.rgb, vec3(2.2));
    } else {
        if (materialParams.baseColor.a < 0.5) discard;
        baseColor = materialParams.baseColor.rgb;
    }

    int cascadeIndex = 0;
    for(int i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if(inViewPosition.z < lights.cascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }

    vec4 shadowMapCoord = (biasMat * lights.lightProjection[cascadeIndex]) * inWorldPosition;

    vec3 albedo = fragColor * materialParams.tint.rgb * baseColor;

    float metallic = materialParams.metallic;
    float roughness = materialParams.roughness;
    float occlusion = 1.0;

    if (materialParams.useMetallicRoughnessMap)
    {
        vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);

        occlusion =  metallicRoughness.r;
        metallic =  metallicRoughness.b;
        roughness = metallicRoughness.g;
    }

    vec3 normalMapColor = texture(normalSampler, fragTexCoord).rgb;

    N = materialParams.useNormalMap
    ? CalcSurfaceNormal(normalMapColor, TBN)
    : fragNormal;
    V = normalize(transforms.cameraPosition - inWorldPosition.xyz);

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

    vec2 texSize = textureSize(SSAO, 0);
    vec2 texCoord = ((gl_FragCoord.xy + vec2(1.0)) * 0.5) / texSize;
    float SSAOValue = texture(SSAO, texCoord).r;

    vec3 ambient = lights.ambientIntensity * SSAOValue * (kD * diffuse + specular);

    vec3 color = ambient + Lo;

    color =  color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    //color = vec3(abs(inViewPosition.z * 0.01));
    //color = vec3(0.25 * cascadeIndex);

    //vec2 shadowMapUV = (shadowMapCoord / shadowMapCoord.w).st;
    //float shadowMapValue = texture(shadowMap, vec3(shadowMapUV, 2)).r;

    //finalImage = vec4(vec3(shadowMapValue), 1.0);
    finalImage = vec4(color, 1.0);
}