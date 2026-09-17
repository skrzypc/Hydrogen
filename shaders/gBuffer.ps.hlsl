#include "common.hlsli"
#include "shaderUtils.hlsli"

struct PushConstants
{
    uint transformIndex;
    uint materialIndex;
    uint baseVertex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct PsIn
{
    float4 positionClipSpace : SV_Position;
    float3 normalWorldSpace : NORMAL;
};

struct PsOut
{
    float4 albedo : SV_Target0;
    float2 normal : SV_Target1;
    float2 roughnessMetallic : SV_Target2;
};

PsOut mainPS(PsIn input)
{
    StructuredBuffer<GpuMaterialData> materialDataBuffer = ResourceDescriptorHeap[g_frame.materialDataBufferIndex];

    GpuMaterialData material = materialDataBuffer[g_push.materialIndex];

    PsOut output;

    output.albedo = float4(material.albedo, 1.0f);
    output.normal = EncodeOctahedral(normalize(input.normalWorldSpace));
    output.roughnessMetallic = float2(material.roughness, material.metallic);

    return output;
}
