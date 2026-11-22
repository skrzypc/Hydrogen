#include "include/common.hlsli"
#include "include/lighting.hlsli"
#include "include/brdf.hlsli"
#include "include/reservoir.hlsli"

struct PushConstants
{
    uint albedoIndex;
    uint normalIndex;
    uint roughnessMetallicIndex;
    uint depthIndex;
    uint reservoirIndex;
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

    RWStructuredBuffer<Reservoir> reservoirBuffer = ResourceDescriptorHeap[g_push.reservoirIndex];

    uint pixelIndex = pixel.y * view.viewportSize.x + pixel.x;
    Reservoir reservoir;
    InitializeReservoir(reservoir);
    
    Texture2D<float> depthTarget = ResourceDescriptorHeap[g_push.depthIndex];
    
    float depth = depthTarget[pixel];
    if (depth <= 0.0f)
    {
        reservoirBuffer[pixelIndex] = reservoir;
        
        return;
    }
   
    Texture2D<float4> albedoTarget = ResourceDescriptorHeap[g_push.albedoIndex];
    Texture2D<float4> normalTarget = ResourceDescriptorHeap[g_push.normalIndex];
    Texture2D<float2> roughnessMetallicTarget = ResourceDescriptorHeap[g_push.roughnessMetallicIndex];
    
    float2 uv = (float2(pixel) + 0.5f) / view.viewportSize;
    float3 surfaceWorldPosition = ReconstructWorldPosition(uv, depth, view.invViewProjectionMx);

    Surface surface;
    
    surface.albedo = albedoTarget[pixel].rgb;
    
    float2 roughnessMetallic = roughnessMetallicTarget[pixel];
    surface.roughness = roughnessMetallic.x;
    surface.metallic = roughnessMetallic.y;

    float3 N = DecodeOctahedral(normalTarget[pixel].xy);
    float3 V = normalize(view.worldPosition - surfaceWorldPosition);

    RngState rngState = InitRng(pixel, view.viewportSize, g_frame.frameNumber);
    
    StructuredBuffer<GpuLight> lightsBuffer = ResourceDescriptorHeap[g_frame.lightBufferIndex];

    const uint candidateCount = 8;
    float selectedPHat = 0.0f;
    for (uint i = 0; i < candidateCount; ++i)
    {
        GpuLight candidateLight;
        float candidateLightWeight; // Weight is 1 / pdf.
        uint candidateLightIndex = 0xffffffff;

        if (SampleLightUniformly(lightsBuffer, rngState, candidateLight, candidateLightWeight, candidateLightIndex))
        {
            float3 lightDirection;
            float lightDistance;
            GetLightDirectionAndDistance(candidateLight, surfaceWorldPosition, lightDirection, lightDistance);

            float3 lightIrradiance = GetLightIrradiancePT(candidateLight, lightDirection, lightDistance);
#if 1
            float3 materialBrdf = EvaluateBrdfGgx(surface, N, V, lightDirection); // GGX
            float NoL = saturate(dot(N, lightDirection));

            float3 radiance = materialBrdf * lightIrradiance * NoL;

            float candidatePHat = CalculateLuminance(radiance);
#else
            float candidatePHat = CalculateLuminance(GetLightIrradiancePT(candidateLight, lightDirection, lightDistance));
#endif
            
            const float candidateRisWeight = candidatePHat * candidateLightWeight; // pHat / pdf

            if (UpdateReservoir(reservoir, candidateLightIndex, candidateRisWeight, rngState))
            {
                selectedPHat = candidatePHat;
            }
        }
    }

    reservoir.contributionWeight = selectedPHat > 0.0f ?
        reservoir.weightSum / (float(reservoir.sampleCount) * selectedPHat) :
        0.0f;

    reservoirBuffer[pixelIndex] = reservoir;
}
