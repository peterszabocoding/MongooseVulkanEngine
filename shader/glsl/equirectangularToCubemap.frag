#version 450

layout(location = 0) in vec3 inWorldPosition;

layout(binding = 1) uniform sampler2D equirectangularMap;

layout(location = 0) out vec4 fragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(inWorldPosition));
    vec3 color = texture(equirectangularMap, uv).rgb;

    //color = color / (color + vec3(1.0));
    //color = pow(color, vec3(2.2));

    fragColor = vec4(color, 1.0);
}
