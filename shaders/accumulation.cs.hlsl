#include "include/common.hlsli"

struct PushConstants
{
    uint sceneColorIndex;
    uint accumulationTargetIndex;
    uint accumulatedFramesCount;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

// Keeps a running average of scene color in a persistent target and writes it back into scene color.
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

    RWTexture2D<float4> sceneColor = ResourceDescriptorHeap[g_push.sceneColorIndex];
    RWTexture2D<float4> accumulationTarget = ResourceDescriptorHeap[g_push.accumulationTargetIndex];

    float3 currentSample = sceneColor[pixel].rgb;

    // Running average instead of a sum: stays in a stable range, so late samples keep their precision.
    float3 previousAverage = (g_push.accumulatedFramesCount > 1u) ? accumulationTarget[pixel].rgb : currentSample;
    float3 average = previousAverage + (currentSample - previousAverage) / float(g_push.accumulatedFramesCount);

    accumulationTarget[pixel] = float4(average, 1.0f);
    sceneColor[pixel] = float4(average, 1.0f);
}
