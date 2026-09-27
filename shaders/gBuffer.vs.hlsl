#include "include/common.hlsli"

struct PushConstants
{
    uint transformIndex;
    uint materialIndex;
    uint baseVertex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct VsOut
{
    float4 positionClipSpace : SV_Position;
    float3 normalWorldSpace : NORMAL;
    float4 tangentWorldSpace : TANGENT;
    float2 uvCoords : TEXCOORD0;
};

VsOut mainVS(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[g_frame.vertexNormalBufferIndex];
    StructuredBuffer<float4> tangents = ResourceDescriptorHeap[g_frame.vertexTangentBufferIndex];
    StructuredBuffer<float2> uvs = ResourceDescriptorHeap[g_frame.vertexUvBufferIndex];
    StructuredBuffer<float4x4> transforms = ResourceDescriptorHeap[g_frame.transformBufferIndex];
    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];

    uint globalVertex = vertexID + g_push.baseVertex;
    float3 position = positions[globalVertex];
    float3 normal = normals[globalVertex];
    float4 tangent = tangents[globalVertex];
    float4x4 worldMx = transforms[g_push.transformIndex];
    float4x4 viewProjectionMx = views[g_frame.mainViewIndex].viewProjectionMx;

    float4 worldPosition = mul(worldMx, float4(position, 1.0f));

    VsOut output;
    output.positionClipSpace = mul(viewProjectionMx, worldPosition);
    output.normalWorldSpace = mul((float3x3)worldMx, normal);
    output.tangentWorldSpace = float4(mul((float3x3)worldMx, tangent.xyz), tangent.w);
    output.uvCoords = uvs[globalVertex];

    return output;
}
