#version 450

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 modelMatrix;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;

layout(location = 5) out mat3 TBN;


void main() {
    vec4 worldPosition = push.transform * vec4(inPosition, 1.0);
    fragPosition = worldPosition.xyz / worldPosition.w;

    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = normalize(transpose(inverse(mat3(push.modelMatrix))) * inNormal);
    fragTangent = inTangent;

    vec3 N = normalize(vec3(push.modelMatrix * vec4(inNormal, 0.0)));
    vec3 T = normalize(vec3(push.modelMatrix * vec4(inTangent, 0.0)));
    vec3 B = normalize(vec3(push.modelMatrix * vec4(inBitangent, 0.0)));
    TBN = mat3(T, B, N);

    gl_Position = worldPosition;
}
