#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outputImage;

layout(set = 0, binding = 0) uniform sampler2D imageSampler;

layout(push_constant) uniform Push {
    float exposure;
    float gamma;
} params;

void main() {
    vec3 hdrColor = texture(imageSampler, inTexCoord).rgb;

    vec3 mapped = vec3(1.0) - exp(-hdrColor * params.exposure);

    mapped = pow(mapped, vec3(1.0 / params.gamma));

    outputImage = vec4(mapped, 1.0);
}