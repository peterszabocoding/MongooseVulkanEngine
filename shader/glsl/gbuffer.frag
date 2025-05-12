#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

#include <shadow_mapping.glslh>
#include <pbr_functions.glslh>

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec4 inWorldPosition;
layout(location = 6) in vec4 shadowMapCoord;
layout(location = 7) in mat3 TBN;

layout(set = 0, binding = 0) uniform MaterialParams {
    vec4 tint;
    vec4 baseColor;
    float metallic;
    float roughness;

    bool useBaseColorMap;
    bool useNormalMap;
    bool useMetallicRoughnessMap;
} materialParams;

layout(set = 1, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;

layout(std430, set = 3, binding = 0) uniform Lights {
    mat4 lightView;
    mat4 lightProjection;
    vec3 direction;
    float ambientIntensity;
    vec4 color;
    float intensity;
    float bias;
} lights;

layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessSampler;

layout(set = 2, binding = 0) uniform samplerCube skyboxSampler;
layout(set = 4, binding = 0) uniform samplerCube irradianceMap;

layout(set = 3, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) out vec4 finalImage;
layout(location = 1) out vec4 baseColorImage;
layout(location = 2) out vec4 normalImage;
layout(location = 3) out vec4 metallicRoughnessImage;
layout(location = 4) out vec4 outWorldPosition;

vec3 N;
vec3 V;

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
    normal.xy = normalFromTexture.rg;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normalize(TBN * normal);
}

vec3 CalcDirectionalLightRadiance(vec3 direction)
{
    vec3 lightDir = direction;
    float diffuseFactor = clamp(dot(N, lightDir), 0.0, 1.0);

    float shadowCoeff = filterPCF(shadowMap, shadowMapCoord / shadowMapCoord.w);
    return shadowCoeff * lights.intensity * lights.color.rgb * diffuseFactor;
}

void main() {

    vec4 baseColor = materialParams.useBaseColorMap ? texture(baseColorSampler, fragTexCoord) : materialParams.baseColor;

    vec4 albedo = vec4(fragColor, 1.0) * materialParams.tint * baseColor;
    baseColorImage = albedo;

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
    F0      = mix(F0, albedo.rgb, metallic);

    //float distance    = length(lightPosition - inWorldPosition.xyz);
    //float attenuation = 1.0 / (distance * distance);
    //vec3 radiance     = lights.intensity * lights.color.rgb * attenuation;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    albedo = vec4(irradiance * albedo.rgb, 1.0);

    vec3 radiance = CalcDirectionalLightRadiance(-lights.direction);
    vec3 Lo = CalcLightRadiance(L, H, V, N, F0, albedo.rgb, roughness, metallic, radiance);

    vec3 ambient = lights.ambientIntensity * albedo.rgb;
    vec3 color = ambient + Lo;

    /////////////////   Reflection   /////////////////////
    vec3 I = normalize(fragPosition - transforms.cameraPosition);
    vec3 R = reflect(I, N);
    vec3 reflectionColor = texture(skyboxSampler, R).rgb;
    /////////////////////////////////////////////////////

    /////////////////   GBuffer   ////////////////////////
    normalImage = vec4(N, 1.0);
    metallicRoughnessImage = vec4(occlusion, metallic, roughness, 1.0);
    outWorldPosition = inWorldPosition;
    /////////////////////////////////////////////////////

    color += 0.1 * (1.0 - roughness) * reflectionColor;

    //float shadowCoeff = filterPCF(shadowMap, shadowMapCoord / shadowMapCoord.w);
    //finalImage = (lights.ambientIntensity + shadowCoeff * lights.intensity * diffuseFactor) * albedo;
    finalImage = vec4(color, 1.0);
}