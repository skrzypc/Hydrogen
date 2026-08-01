#include "common.hlsli"

struct PushConstants
{
    uint transformIndex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct PsIn
{
    float4 posCS    : SV_Position;
    float3 posWS    : POSITION;
    float3 normalWS : NORMAL;
};

static const float3 kPalette[4] =
{
    float3(1.0f, 0.65f, 0.25f), // orange (bunny)
    float3(0.55f, 0.55f, 0.58f), // gray   (floor)
    float3(0.40f, 0.75f, 0.45f), // green
    float3(0.65f, 0.45f, 0.80f), // purple
};

float3 LinearToSrgb(float3 linearColor)
{
    linearColor = max(linearColor, 0.0f);

    float3 low = linearColor * 12.92f;
    float3 high = 1.055f * pow(linearColor, 1.0f / 2.4f) - 0.055f;

    return lerp(low, high, step(0.0031308f, linearColor));
}

// Returns illuminance at the surface, in the light's authored units.
float3 EvaluateLight(GpuLight light, float3 positionWS, float3 normal)
{
    float3 toLight;
    float attenuation = 1.0f;

    if (light.type == LightTypeDirectional)
    {
        toLight = -light.direction;
    }
    else
    {
        float3 delta = light.position - positionWS;
        float distanceSquared = max(dot(delta, delta), 1e-6f);
        float distance = sqrt(distanceSquared);

        if (distance > light.range)
        {
            return float3(0.0f, 0.0f, 0.0f);
        }

        toLight = delta / distance;

        attenuation = 1.0f / distanceSquared;

        if (light.type == LightTypeSpot)
        {
            float cosAngle = dot(-toLight, light.direction);
            float coneFalloff = max(light.cosInnerConeAngle - light.cosOuterConeAngle, 1e-4f);
            attenuation *= saturate((cosAngle - light.cosOuterConeAngle) / coneFalloff);
        }
    }

    return light.color * light.intensity * attenuation * saturate(dot(normal, toLight));
}

float4 mainPS(PsIn input) : SV_Target
{
    float3 albedo = kPalette[g_push.transformIndex % 4];
    float3 normal = normalize(input.normalWS);

    StructuredBuffer<GpuLight> lights = ResourceDescriptorHeap[g_frame.lightBufferIndex];

    float3 illuminance = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < g_frame.lightCount; ++i)
    {
        illuminance += EvaluateLight(lights[i], input.posWS, normal);
    }

    // Lambert
    float3 radiance = albedo * illuminance / 3.14159265f;

    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    float exposure = views[g_frame.mainViewIndex].exposure;

    return float4(LinearToSrgb(radiance * exposure), 1.0f);
}
