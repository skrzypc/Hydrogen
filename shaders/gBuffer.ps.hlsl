#include "common.hlsli"
#include "octahedral.hlsli"

struct PushConstants
{
    uint transformIndex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct PsIn
{
    float4 posCS    : SV_Position;
    float3 normalWS : NORMAL;
};

struct PsOut
{
    float4 albedo             : SV_Target0;
    float2 normal             : SV_Target1;
    float2 roughnessMetalness : SV_Target2;
};

static const float3 kPalette[4] =
{
    float3(1.0f, 0.65f, 0.25f),
    float3(0.55f, 0.55f, 0.58f),
    float3(0.40f, 0.75f, 0.45f),
    float3(0.65f, 0.45f, 0.80f)
};

PsOut mainPS(PsIn input)
{
    PsOut output;

    output.albedo = float4(kPalette[g_push.transformIndex % 4], 1.0f);
    output.normal = EncodeOctahedral(normalize(input.normalWS));

    // No materials yet, so everything is a rough dielectric.
    output.roughnessMetalness = float2(0.8f, 0.0f);

    return output;
}
