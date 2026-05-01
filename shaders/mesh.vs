#include "common.hlsli"

struct PushConstants
{
    float3 color;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

float4 mainVS(uint vertexID : SV_VertexID) : SV_Position
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    float3 pos = positions[vertexID];

    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    float4x4 vp = views[g_frame.mainViewIndex].viewProjectionMx;

    return mul(vp, float4(pos, 1.0f));
}
