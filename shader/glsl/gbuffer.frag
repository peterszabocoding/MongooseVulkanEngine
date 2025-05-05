#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in mat3 TBN;

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
    vec4 cameraPosition;
    mat4 view;
    mat4 projection;
} transforms;

layout(set = 3, binding = 0) uniform Lights {
    vec3 direction;
    vec4 ambientColor;
    float ambientIntensity;
} lights;

layout(set = 0, binding = 1) uniform sampler2D baseColorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessSampler;

layout(set = 2, binding = 0) uniform samplerCube skyboxSampler;


layout(location = 0) out vec4 finalImage;
layout(location = 1) out vec4 baseColorImage;
layout(location = 2) out vec4 normalImage;
layout(location = 3) out vec4 metallicRoughnessImage;
layout(location = 4) out vec4 worldPosition;

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

    vec4 diffuseColor = vec4(fragColor, 1.0) * materialParams.tint * color;


    /////////////////   Reflection   /////////////////////
    vec3 I = normalize(fragPosition - transforms.cameraPosition.xyz);
    vec3 R = reflect(I, normalWorldSpace);
    vec3 reflectionColor = texture(skyboxSampler, R).rgb;
    /////////////////////////////////////////////////////

    /////////////////   GBuffer   ////////////////////////
    baseColorImage = diffuseColor;
    normalImage = vec4(normalWorldSpace, 1.0);
    metallicRoughnessImage = vec4(occlusion, metallic, roughness, 1.0);
    worldPosition = vec4(fragPosition, 1.0);
    /////////////////////////////////////////////////////

    diffuseColor += 0.05 * vec4(reflectionColor, 1.0);
    float diffuseFactor = clamp(dot(normalWorldSpace, lights.direction), 0.0, 1.0);
    finalImage = lights.ambientIntensity * lights.ambientColor + diffuseFactor * diffuseColor;
}