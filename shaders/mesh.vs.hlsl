#include "common.hlsli"

struct PushConstants
{
    uint transformIndex;
    uint baseVertex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct VsOut
{
    float4 posCS    : SV_Position;
    float3 normalWS : NORMAL;
};

VsOut mainVS(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    StructuredBuffer<float3> normals   = ResourceDescriptorHeap[g_frame.vertexNormalBufferIndex];
    StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[g_frame.transformBufferIndex];
    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];

    uint globalVertex = vertexID + g_push.baseVertex;
    float3 pos    = positions[globalVertex];
    float3 normal = normals[globalVertex];
    float4x4 world = transforms[g_push.transformIndex];
    float4x4 vp    = views[g_frame.mainViewIndex].viewProjectionMx;

    VsOut o;
    o.posCS    = mul(vp, mul(world, float4(pos, 1.0f)));
    o.normalWS = mul((float3x3)world, normal);
    return o;
}
