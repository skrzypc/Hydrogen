#include "include/common.hlsli"
#include "include/lighting.hlsli"
#include "include/shaderUtils.hlsli"
#include "include/brdf.hlsli"

struct PushConstants
{
    uint albedoIndex;
    uint normalIndex;
    uint roughnessMetallicIndex;
    uint depthIndex;
    uint outputIndex;
    uint indexToOutput;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

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
    Texture2D<float2> roughnessMetallicTarget = ResourceDescriptorHeap[g_push.roughnessMetallicIndex];
    Texture2D<float> depthTarget = ResourceDescriptorHeap[g_push.depthIndex];
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

    if (g_push.indexToOutput == 0)
    {
        output[pixel] = albedoTarget[pixel];
    }
    else if (g_push.indexToOutput == 1)
    {
        output[pixel] = float4(DecodeOctahedral(normalTarget[pixel].rg), 1.0f);
    }
    else if (g_push.indexToOutput == 2)
    {
        output[pixel] = float4(roughnessMetallicTarget[pixel].r, 0.0f, 0.0f, 1.0f);
    }
    else if (g_push.indexToOutput == 3)
    {
        output[pixel] = float4(0.0f, roughnessMetallicTarget[pixel].g, 0.0f, 1.0f);
    }
    else if (g_push.indexToOutput == 4)
    {
        float linearZ = view.nearPlane * view.farPlane / (view.nearPlane + depthTarget[pixel] * (view.farPlane - view.nearPlane));
        float depth = saturate(linearZ / 50.0f);
        output[pixel] = float4(depth, depth, depth, 1.0f);
    }

    return;
}
