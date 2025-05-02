#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in mat3 TBN;

layout(binding = 0) uniform MaterialParams {
    vec4 tint;
    vec4 baseColor;
    float metallic;
    float roughness;

    bool useBaseColorMap;
    bool useNormalMap;
    bool useMetallicRoughnessMap;
} materialParams;

layout(binding = 1) uniform sampler2D baseColorSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D metallicRoughnessSampler;

layout(push_constant) uniform Push {
  mat4 transform;
  mat4 modelMatrix;
} push;

layout(location = 0) out vec4 baseColorImage;
layout(location = 1) out vec4 normalImage;
layout(location = 2) out vec4 metallicRoughnessImage;

const vec3 LIGHT_DIRECTION = normalize(vec3(0.0, 1.0, 1.0));
const float AMBIENT = 0.05;

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
    normal.xy = normalFromTexture.rg;
    normal.xy = 2.0 * normal.xy - 1.0;
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normalize(TBN * normal);
}

void main() {

    vec4 color = materialParams.useBaseColorMap ? texture(baseColorSampler, fragTexCoord) : materialParams.baseColor;

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
    vec3 normalWorldSpace = materialParams.useNormalMap
                ? CalcSurfaceNormal(normalMapColor, TBN)
                : fragNormal;

    float diffuseFactor = AMBIENT + clamp(dot(normalWorldSpace, LIGHT_DIRECTION), 0.0, 1.0);
    vec4 diffuseColor = vec4(fragColor, 1.0) * materialParams.tint * color;

    baseColorImage = diffuseColor;
    normalImage = vec4(normalWorldSpace, 1.0);
    metallicRoughnessImage = vec4(occlusion, metallic, roughness, 1.0);
}