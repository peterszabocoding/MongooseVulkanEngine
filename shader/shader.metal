#include <metal_stdlib>
using namespace metal;

struct VertexIn
{
    float3 position;
    float2 texCoord;
};

struct v2f
{
    float4 position [[position]];
    float2 texCoord;
};

v2f vertex vertexMain( uint vertexId [[vertex_id]],
                       device const VertexIn* vertices [[buffer(0)]])
{
    v2f o;
    o.position = float4( vertices[ vertexId ].position, 1.0 );
    o.texCoord = vertices[ vertexId ].texCoord;
    return o;
}

constexpr sampler textureSampler (mag_filter::linear,
                                  min_filter::linear);

half4 fragment fragmentMain( 
    v2f in [[stage_in]],
    texture2d<half> colorTexture [[ texture(0) ]]
    )
{
    // Sample the texture to obtain a color
    const half4 colorSample = colorTexture.sample(textureSampler, in.texCoord);
    return colorSample;
}
