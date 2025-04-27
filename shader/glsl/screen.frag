#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D baseColorSampler;
layout(binding = 1) uniform sampler2D normalSampler;
layout(binding = 2) uniform sampler2D metallicRoughnessSampler;

layout(location = 0) out vec4 finalImage;

const vec3 LIGHT_DIRECTION = normalize(vec3(0.0, 1.0, 1.0));
const float AMBIENT = 0.05;

void main() {
    vec4 baseColor = texture(baseColorSampler, fragTexCoord);
    vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
    vec3 normal = texture(normalSampler, fragTexCoord).xyz;

    float occlusion =  metallicRoughness.r;
    float metallic =  metallicRoughness.b;
    float roughness = metallicRoughness.g;

    float diffuseFactor = AMBIENT + clamp(dot(normal, LIGHT_DIRECTION), 0.0, 1.0);

    finalImage = diffuseFactor * baseColor;
}
