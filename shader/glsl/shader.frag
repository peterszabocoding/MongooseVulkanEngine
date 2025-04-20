#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;

layout(binding = 0) uniform MaterialParams {
    vec3 tint;
    bool useNormalMap;
} materialParams;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D metallicRoughnessSampler;

layout(push_constant) uniform Push {
  mat4 transform;
  mat4 normalMatrix;
} push;

layout(location = 0) out vec4 outColor;

const vec3 LIGHT_DIRECTION = normalize(vec3(0.0, 1.0, 1.0));
const float AMBIENT = 0.05;

vec3 CalcSurfaceNormal(vec3 normalFromTexture, mat3 TBN)
{
    vec3 normal;
	normal = 2.0 * normalFromTexture.xyz - 1.0;
	return normalize(TBN * normal);
}

void main() {

    vec3 q1 = dFdx(fragPosition);
	vec3 q2 = dFdy(fragPosition);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);

	vec3 N = normalize(fragNormal);
	vec3 T = -normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

    vec3 normalWorldSpace = materialParams.useNormalMap
            ? CalcSurfaceNormal(texture(normalSampler, fragTexCoord).rgb, TBN)
            : fragNormal;

    float diffuseFactor = AMBIENT + clamp(dot(normalWorldSpace, normalize(LIGHT_DIRECTION)), 0.0, 1.0);
    vec4 diffuseColor = vec4(materialParams.tint, 1.0) * texture(texSampler, fragTexCoord);
    outColor = diffuseFactor * diffuseColor;
}