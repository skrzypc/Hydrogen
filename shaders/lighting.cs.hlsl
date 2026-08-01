#include "common.hlsli"
#include "octahedral.hlsli"

struct PushConstants
{
    uint albedoIndex;
    uint normalIndex;
    uint roughnessMetalnessIndex;
    uint depthIndex;
    uint outputIndex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

static const float3 kSkyRadiance = float3(0.02f, 0.04f, 0.08f);

float3 ReconstructWorldPosition(float2 uv, float depth, float4x4 invViewProjection)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);

    float4 clipPosition = float4(ndc, depth, 1.0f);
    float4 worldPosition = mul(invViewProjection, clipPosition);

    return worldPosition.xyz / worldPosition.w;
}

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

[numthreads(8, 8, 1)]
void mainCS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    ViewData view = views[g_frame.mainViewIndex];

    uint2 pixel = dispatchThreadId.xy;
    if (any(pixel >= uint2(view.viewportSize)))
    {
        return;
    }

    Texture2D<float4> albedoTarget = ResourceDescriptorHeap[g_push.albedoIndex];
    Texture2D<float2> normalTarget = ResourceDescriptorHeap[g_push.normalIndex];
    Texture2D<float> depthTarget = ResourceDescriptorHeap[g_push.depthIndex];
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

    float depth = depthTarget[pixel];

    float3 radiance = kSkyRadiance;

    // Reversed Z, so the cleared far plane is zero and means nothing was rasterised here.
    if (depth > 0.0f)
    {
        float2 uv = (float2(pixel) + 0.5f) / view.viewportSize;

        float3 positionWS = ReconstructWorldPosition(uv, depth, view.invViewProjectionMx);
        float3 normal = DecodeOctahedral(normalTarget[pixel]);
        float3 albedo = albedoTarget[pixel].rgb;

        StructuredBuffer<GpuLight> lights = ResourceDescriptorHeap[g_frame.lightBufferIndex];

        float3 illuminance = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < g_frame.lightCount; ++i)
        {
            illuminance += EvaluateLight(lights[i], positionWS, normal);
        }

        radiance = albedo * illuminance / 3.14159265f;
    }

    output[pixel] = float4(radiance, 1.0f);
}
