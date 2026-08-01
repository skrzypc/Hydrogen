#include "common.hlsli"
#include "colorSpace.hlsli"
#include "tonemap.hlsli"

struct PushConstants
{
    uint sceneColorIndex;
    uint outputIndex;
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

    Texture2D<float4> sceneColor = ResourceDescriptorHeap[g_push.sceneColorIndex];
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

    float3 radiance = sceneColor[pixel].rgb;

    float3 exposed = radiance * view.exposure;
    float3 tonemapped = TonemapAces(exposed);

    output[pixel] = float4(LinearToSrgb(tonemapped), 1.0f);
}
