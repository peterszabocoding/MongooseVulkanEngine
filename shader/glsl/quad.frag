#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outputImage;

layout(set = 0, binding = 0) uniform sampler2D imageSampler;

void main() {
    outputImage = texture(imageSampler, inTexCoord);
}