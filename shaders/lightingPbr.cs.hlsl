#include "common.hlsli"
#include "lighting.hlsli"
#include "shaderUtils.hlsli"
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

float3 ReconstructWorldPosition(float2 uv, float depth, float4x4 invViewProjection)
{
    float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);

    float4 clipPosition = float4(ndc, depth, 1.0f);
    float4 worldPosition = mul(invViewProjection, clipPosition);

    return worldPosition.xyz / worldPosition.w;
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
        float3 surfacePosition = ReconstructWorldPosition(uv, depth, view.invViewProjectionMx);

        float2 roughnessMetalness = roughnessMetalnessTarget[pixel];

        Surface surface;
        surface.albedo = albedoTarget[pixel].rgb;
        surface.roughness = roughnessMetalness.x;
        surface.metalness = roughnessMetalness.y;

        float3 N = DecodeOctahedral(normalTarget[pixel]);
        float3 V = normalize(view.worldPosition - surfacePosition);

        StructuredBuffer<GpuLight> lights = ResourceDescriptorHeap[g_frame.lightBufferIndex];

        radiance = float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < g_frame.lightCount; ++i)
        {
            float3 lightDirection;
            float lightDistance;
            GetLightDirectionAndDistance(lights[i], surfacePosition, lightDirection, lightDistance);

            float NoL = saturate(dot(N, lightDirection));
            if (NoL <= 0.0f)
            {
                continue;
            }

            float3 lightRadiance = GetLightContribution(lights[i], lightDirection, lightDistance);
            radiance += lightRadiance * EvaluateBrdf(surface, N, V, lightDirection) * NoL;
        }
    }

    output[pixel] = float4(radiance, 1.0f);
}
