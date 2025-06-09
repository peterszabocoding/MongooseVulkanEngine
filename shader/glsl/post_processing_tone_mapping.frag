#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outputImage;

layout(set = 0, binding = 0) uniform sampler2D imageSampler;

void main() {

    vec3 color = texture(imageSampler, inTexCoord).rgb;
    color =  color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    outputImage = vec4(color, 1.0);

}