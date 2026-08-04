#include "common.hlsli"
#include "octahedral.hlsli"
#include "brdf.hlsli"

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
static const float kRangeFalloffStartFraction = 0.8f;

struct LightSample
{
    float3 radiance;
    float3 direction;
};

float3 ReconstructWorldPosition(float2 uv, float depth, float4x4 invViewProjection)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);

    float4 clipPosition = float4(ndc, depth, 1.0f);
    float4 worldPosition = mul(invViewProjection, clipPosition);

    return worldPosition.xyz / worldPosition.w;
}

LightSample SampleLight(GpuLight light, float3 positionWS)
{
    LightSample result;
    result.radiance = float3(0.0f, 0.0f, 0.0f);
    result.direction = float3(0.0f, 0.0f, 0.0f);

    float attenuation = 1.0f;
    float softRangeFalloffFactor = 1.0f;

    if (light.type == LightTypeDirectional)
    {
        result.direction = -light.direction;
    }
    else
    {
        float3 delta = light.position - positionWS;
        float distanceSquared = max(dot(delta, delta), 1e-6f);
        float distance = sqrt(distanceSquared);

        if (distance > light.range)
        {
            return result;
        }

        float softRangeFalloffStart = light.range * kRangeFalloffStartFraction;
        float softRangeFalloffEnd = light.range;
        float softRangeFalloffWidth = max(softRangeFalloffEnd - softRangeFalloffStart, 1e-4f);
        float softRangeFalloff = saturate(1.0f - ((distance - softRangeFalloffStart) / softRangeFalloffWidth));

        softRangeFalloffFactor = softRangeFalloff * softRangeFalloff;
        
        result.direction = delta / distance;

        attenuation = 1.0f / distanceSquared;

        if (light.type == LightTypeSpot)
        {
            float cosAngle = dot(-result.direction, light.direction);
            float coneFalloff = max(light.cosInnerConeAngle - light.cosOuterConeAngle, 1e-4f);
            attenuation *= saturate((cosAngle - light.cosOuterConeAngle) / coneFalloff);
        }
    }

    result.radiance = light.color * light.intensity * attenuation * softRangeFalloffFactor;

    return result;
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
    Texture2D<float2> roughnessMetalnessTarget = ResourceDescriptorHeap[g_push.roughnessMetalnessIndex];
    Texture2D<float> depthTarget = ResourceDescriptorHeap[g_push.depthIndex];
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

    float depth = depthTarget[pixel];

    float3 radiance = kSkyRadiance;

    // If not sky.
    if (depth > 0.0f)
    {
        float2 uv = (float2(pixel) + 0.5f) / view.viewportSize;
        float3 positionWS = ReconstructWorldPosition(uv, depth, view.invViewProjectionMx);

        float2 roughnessMetalness = roughnessMetalnessTarget[pixel];

        Surface surface;
        surface.albedo = albedoTarget[pixel].rgb;
        surface.roughness = roughnessMetalness.x;
        surface.metalness = roughnessMetalness.y;

        float3 N = DecodeOctahedral(normalTarget[pixel]);
        float3 V = normalize(view.worldPosition - positionWS);

        StructuredBuffer<GpuLight> lights = ResourceDescriptorHeap[g_frame.lightBufferIndex];

        radiance = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < g_frame.lightCount; ++i)
        {
            LightSample lightSample = SampleLight(lights[i], positionWS);

            float NoL = saturate(dot(N, lightSample.direction));
            if (NoL <= 0.0f)
            {
                continue;
            }

            radiance += lightSample.radiance * EvaluateBrdf(surface, N, V, lightSample.direction) * NoL;
        }
    }

    output[pixel] = float4(radiance, 1.0f);
}
