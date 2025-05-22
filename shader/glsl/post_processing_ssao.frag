#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shading_language_include : require

layout(location = 0) in vec2 TexCoords;

layout(location = 0) out float FragColor;

// GBuffer
layout(set = 0, binding = 0) uniform sampler2D gNormal;
layout(set = 0, binding = 1) uniform sampler2D gPosition;
layout(set = 0, binding = 2) uniform sampler2D gDepth;

// SSAO
layout(set = 1, binding = 0) uniform SSAOParams {
    vec4 samples[64];
} ssaoParams;

layout(set = 1, binding = 1) uniform sampler2D texNoise;

layout(set = 2, binding = 0) uniform Transforms {
    mat4 projection;
    mat4 view;
    vec3 cameraPosition;
} transforms;


// tile noise texture over screen, based on screen dimensions divided by noise size
layout(push_constant) uniform Push {
    vec2 resolution;
    int kernelSize;
    float radius;
    float bias;
    float strength;
} push;


void main()
{
    vec4 normalSampled = texture(gNormal, TexCoords);
    if (normalSampled.a < 0.5)
    {
        FragColor = 1.0;
        return;
    }

    vec3 normal    = normalSampled.rgb;

    vec3 fragPos   = texture(gPosition, TexCoords).xyz;
    vec3 randomVec = texture(texNoise, TexCoords * push.resolution).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);


    float occlusion = 0.0;
    for(int i = 0; i < push.kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * ssaoParams.samples[i].xyz; // from tangent to view-space
        samplePos = fragPos + samplePos * push.radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset      = transforms.projection * offset;    // from view to clip-space
        offset.xyz /= offset.w;               // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        vec3 sampleFragPos = texture(gPosition, offset.xy).xyz;
        float sampleDepth = sampleFragPos.z;

        float rangeCheck = smoothstep(0.0, 1.0, push.radius / abs(fragPos.z - sampleDepth));
        occlusion       += (sampleDepth >= samplePos.z + push.bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / push.kernelSize);
    FragColor = pow(occlusion, push.strength);
}
