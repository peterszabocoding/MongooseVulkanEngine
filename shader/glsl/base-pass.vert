#version 450
#extension GL_EXT_scalar_block_layout : require

layout(push_constant) uniform Push {
    mat4 transform;
    mat4 modelMatrix;
} push;

layout(set = 1, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragColor;
layout(location = 2) out vec2 outFragTexCoord;
layout(location = 3) out vec3 outFragNormal;
layout(location = 4) out vec4 outWorldPosition;
layout(location = 5) out vec3 outViewPosition;
layout(location = 6) out mat3 TBN;


void main() {
    vec4 worldPosition = push.modelMatrix * vec4(inPosition, 1.0);
    vec4 viewPosition = transforms.view * vec4(worldPosition.xyz, 1.0);

    outWorldPosition = worldPosition;
    outViewPosition = viewPosition.xyz;
    outFragPosition = worldPosition.xyz;

    outFragColor = inColor;
    outFragTexCoord = inTexCoord;
    outFragNormal = normalize(transpose(inverse(mat3(push.modelMatrix))) * inNormal);

    vec3 N = normalize(vec3(push.modelMatrix * vec4(inNormal, 0.0)));
    vec3 T = normalize(vec3(push.modelMatrix * vec4(inTangent, 0.0)));
    vec3 B = normalize(vec3(push.modelMatrix * vec4(inBitangent, 0.0)));
    TBN = mat3(T, B, N);

    gl_Position = transforms.projection * viewPosition;
}
