#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform Push {
  mat4 transform;
  mat4 normalMatrix;
} push;

const vec3 LIGHT_DIRECTION = normalize(vec3(1.0, 1.0, 1.0));
const float AMBIENT = 0.05;

void main() {
    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * fragNormal);

    float diffuseFactor = AMBIENT + clamp(dot(normalWorldSpace, normalize(LIGHT_DIRECTION)), 0.0, 1.0);
    outColor = diffuseFactor * texture(texSampler, fragTexCoord);

}