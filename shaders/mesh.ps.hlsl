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

static const float3 kPalette[4] =
{
    float3(1.0f, 0.65f, 0.25f), // orange (bunny)
    float3(0.55f, 0.55f, 0.58f), // gray   (floor)
    float3(0.40f, 0.75f, 0.45f), // green
    float3(0.65f, 0.45f, 0.80f), // purple
};

float4 mainPS(PsIn input) : SV_Target
{
    float3 baseColor = kPalette[g_push.transformIndex % 4];

    float3 n = normalize(input.normalWS);
    float3 lightDir = normalize(float3(0.3f, 1.0f, 0.5f));
    float ndotl = saturate(dot(n, lightDir));
    float shading = ndotl * 0.85f + 0.15f;

    return float4(baseColor * shading, 1.0f);
}
