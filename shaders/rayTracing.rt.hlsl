#include "common.hlsli"

struct PushConstants
{
    uint tlasIndex;
    uint outputUavIndex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct [raypayload] RayPayload
{
    float3 radiance : write(closesthit, miss) : read(caller);
};

float3 DebugColorFromId(uint id)
{
    uint hash = id * 2654435761u;
    return float3(
        float((hash >> 0) & 0xFF) / 255.0f,
        float((hash >> 8) & 0xFF) / 255.0f,
        float((hash >> 16) & 0xFF) / 255.0f);
}

RayDesc GenerateCameraRay(const float2 vfPixel)
{
    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    const ViewData view = views[g_frame.mainViewIndex];
    
    RayDesc wsRay;
    wsRay.TMin = abs(view.nearPlane);
    wsRay.TMax = abs(view.farPlane);
    wsRay.Origin = view.worldPosition;
    
    float fAspectRatio = view.projectionMx[1][1] / view.projectionMx[0][0];
    float fTanHalfFovY = 1.0f / view.projectionMx[1][1];
    
    // Right: view.viewMx[0].xyz
    // Up: view.viewMx[1].xyz
    // Forward: view.viewMx[2].xyz
    wsRay.Direction = normalize((vfPixel.x * view.viewMx[0].xyz * fTanHalfFovY * fAspectRatio) - (vfPixel.y * view.viewMx[1].xyz * fTanHalfFovY) + view.viewMx[2].xyz);
    
    return wsRay;
}

[shader("raygeneration")]
void mainRayGen()
{  
    const float2 vfResolution = float2(DispatchRaysDimensions().xy);
    const float2 vfPixel = ((float2(DispatchRaysIndex().xy) + 0.5f) / vfResolution * 2.0f) - 1.0f;
    
    RayDesc sWsRay = GenerateCameraRay(vfPixel);
    
    RaytracingAccelerationStructure sTlas = ResourceDescriptorHeap[g_push.tlasIndex];
    
    RayPayload sPayload;
    TraceRay(
        sTlas, // tlas
        RAY_FLAG_FORCE_OPAQUE, // flags
        0xFF, // instance mask
        0, // hit group offset (contributionToHitGroupIndex)
        1, // geometry multiplier (stride, usually 1 hit group per geometry)
        0, // miss shader index
        sWsRay, // the RayDesc from GenerateCameraRay
        sPayload // payload
    );
    
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.outputUavIndex];
    output[DispatchRaysIndex().xy] = float4(sPayload.radiance, 1.0f);
}

[shader("miss")]
void mainMiss(inout RayPayload payload)
{
    payload.radiance = float3(0.0f, 0.0f, 0.0f);
}

[shader("closesthit")]
void mainClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attrs)
{
    payload.radiance = DebugColorFromId(InstanceID());
}
