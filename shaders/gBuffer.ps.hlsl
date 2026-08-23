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

// Placeholder variation until real materials land, so the BRDF has something to show.
static const float kRoughness[4] = { 0.1f, 0.35f, 0.6f, 0.9f };
static const float kMetalness[4] = { 0.0f, 1.0f, 0.0f, 0.0f };

PsOut mainPS(PsIn input)
{
    uint materialIndex = g_push.transformIndex % 4;

    PsOut output;

    //output.albedo = float4(kPalette[materialIndex], 1.0f);
    //output.normal = EncodeOctahedral(normalize(input.normalWS));
    //output.roughnessMetalness = float2(kRoughness[materialIndex], kMetalness[materialIndex]);
    
    output.albedo = float4(1.0, 1.0, 1.0, 1.0f);
    output.normal = EncodeOctahedral(normalize(input.normalWS));
    output.roughnessMetalness = float2(0.1f, 0.1f);

    return output;
}
