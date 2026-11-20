#include "include/common.hlsli"
#include "include/lighting.hlsli"
#include "include/brdf.hlsli"
#include "include/reservoir.hlsli"

struct PushConstants
{
    uint tlasIndex;
    uint albedoIndex;
    uint normalIndex;
    uint roughnessMetallicIndex;
    uint depthIndex;
    uint reservoirIndex;
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

    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputIndex];

    StructuredBuffer<Reservoir> reservoirBuffer = ResourceDescriptorHeap[g_push.reservoirIndex];
    
    uint pixelIndex = pixel.y * view.viewportSize.x + pixel.x;
    Reservoir reservoir = reservoirBuffer[pixelIndex];
    
    Texture2D<float> depthTarget = ResourceDescriptorHeap[g_push.depthIndex];
    
    float depth = depthTarget[pixel];
    if (depth <= 0.0f)
    {
        output[pixel] = float4(kSkyRadiance, 1.0f);
        
        return;
    }

    if (reservoir.contributionWeight <= 0.0f || reservoir.lightIndex >= g_frame.lightCount)
    {
        output[pixel] = float4(0.0f, 0.0f, 0.0f, 1.0f);

        return;
    }

    Texture2D<float4> albedoTarget = ResourceDescriptorHeap[g_push.albedoIndex];
    Texture2D<float2> normalTarget = ResourceDescriptorHeap[g_push.normalIndex];
    Texture2D<float2> roughnessMetallicTarget = ResourceDescriptorHeap[g_push.roughnessMetallicIndex];
    
    float2 uv = (float2(pixel) + 0.5f) / view.viewportSize;
    float3 surfaceWorldPosition = ReconstructWorldPosition(uv, depth, view.invViewProjectionMx);

    Surface surface;
    
    surface.albedo = albedoTarget[pixel].rgb;
    
    float2 roughnessMetallic = roughnessMetallicTarget[pixel];
    surface.roughness = roughnessMetallic.x;
    surface.metallic = roughnessMetallic.y;

    float3 N = DecodeOctahedral(normalTarget[pixel]);
    float3 V = normalize(view.worldPosition - surfaceWorldPosition);

    StructuredBuffer<GpuLight> lightsBuffer = ResourceDescriptorHeap[g_frame.lightBufferIndex];
    RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[g_push.tlasIndex];
    
    GpuLight selectedLight = lightsBuffer.Load(reservoir.lightIndex);

    float3 lightDirection;
    float lightDistance;
    GetLightDirectionAndDistance(selectedLight, surfaceWorldPosition, lightDirection, lightDistance);

    // Check occlusion.
    RayDesc shadowRay;
    shadowRay.Origin = OffsetRayOrigin(surfaceWorldPosition, N);
    shadowRay.Direction = lightDirection;
    shadowRay.TMin = 0.0f;
    shadowRay.TMax = lightDistance > 0.0f ? lightDistance * 0.999f : 100000.0f;

    RayQuery<RAY_FLAG_FORCE_OPAQUE|RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(tlas, RAY_FLAG_NONE, 0xFF, shadowRay);
    rayQuery.Proceed();

    bool occlusion = (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT);

    float3 radiance = float3(0.0f, 0.0f, 0.0f);

    // Light unoccluded, calculate total contribution for given point on surface.
    if (!occlusion)
    {
        float3 lightIrradiance = GetLightIrradiancePT(selectedLight, lightDirection, lightDistance);
        float3 materialBrdf = EvaluateBrdfGgx(surface, N, V, lightDirection); // GGX
        float NoL = saturate(dot(N, lightDirection));

        radiance = (materialBrdf * lightIrradiance * NoL * reservoir.contributionWeight);
    }

    output[pixel] = float4(radiance, 1.0f);
}
