#include "common.hlsli"

struct PushConstants
{
    uint transformIndex;
    float3 color;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

float4 mainVS(uint vertexID : SV_VertexID) : SV_Position
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    float3 pos = positions[vertexID];

    StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[g_frame.transformBufferIndex];
    float4x4 world = transforms[g_push.transformIndex];

    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    float4x4 vp = views[g_frame.mainViewIndex].viewProjectionMx;

    return mul(vp, mul(world, float4(pos, 1.0f)));
}
