#include "common.hlsli"
#include "rng.hlsli"

struct PushConstants
{
    uint tlasIndex;
    uint outputUavIndex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct [raypayload] RayPayload
{
    float3 radiance : write(closesthit, miss) : read(caller);
    bool bounce : write(caller) : read(closesthit);
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

// Based of https://jcgt.org/published/0006/01/01/
void OrthonormalBasis(const float3 normal, out float3 tangent, out float3 bitangent)
{
    const float s = (normal.z >= 0.0f) ? 1.0f : -1.0f;
    const float a = -1.0f / (s + normal.z);
    const float b = normal.x * normal.y * a;
    
    tangent = float3(1.0f + s * normal.x * normal.x * a, s * b, -s * normal.x);
    bitangent = float3(b, s + normal.y * normal.y * a, -normal.y);

    return;
}

[shader("raygeneration")]
void mainRayGen()
{
    const float2 vfResolution = float2(DispatchRaysDimensions().xy);
    const float2 vfPixel = ((float2(DispatchRaysIndex().xy) + 0.5f) / vfResolution * 2.0f) - 1.0f;
    
    RayDesc sWsRay = GenerateCameraRay(vfPixel);
    
    RaytracingAccelerationStructure sTlas = ResourceDescriptorHeap[g_push.tlasIndex];
    
    RayPayload sPayload;
    sPayload.bounce = true;
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
void mainMiss(inout RayPayload sPayload)
{
    sPayload.radiance = float3(0.0f, 0.0f, 0.0f);
}

[shader("closesthit")]
void mainClosestHit(inout RayPayload sPayload, in BuiltInTriangleIntersectionAttributes attrs)
{
    if (InstanceIndex() == 0 && sPayload.bounce)
    {
        RayPayload sReflectionPayload;
        sReflectionPayload.bounce = false;
        
        RngState rngState = InitRng(uint2(DispatchRaysIndex().xy), DispatchRaysDimensions().xy, g_frame.frameNumber);
        
        // cosine weighted hemisphere sampling
        float u1 = 0.5f; // NextRandomFloat(rngState);
        float u2 = 0.5f; // NextRandomFloat(rngState);
        
        float r = sqrt(u1);
        float phi = 2.0f * 3.14159265359f * u2;
        
        float x, y, z;
        sincos(phi, z, x);
        x *= r;
        z *= r;
        y = sqrt(max(0.0f, 1.0f - u1));
        
        float3 normal = float3(0.0f, 1.0f, 0.0f);
        float3 tangent, bitangent;
        OrthonormalBasis(normal, tangent, bitangent);
        
        RayDesc reflectionRay;
        reflectionRay.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        reflectionRay.Direction = normalize(x * tangent + y * normal + z * bitangent); //reflect(WorldRayDirection(), float3(0.0f, 1.0f, 0.0f));
        reflectionRay.TMin = 0.01f;
        reflectionRay.TMax = 1000000.0f;
        
        RaytracingAccelerationStructure sTlas = ResourceDescriptorHeap[g_push.tlasIndex];
        
        TraceRay(
            sTlas, // tlas
            RAY_FLAG_FORCE_OPAQUE, // flags
            0xFF, // instance mask
            0, // hit group offset (contributionToHitGroupIndex)
            1, // geometry multiplier (stride, usually 1 hit group per geometry)
            0, // miss shader index
            reflectionRay, // the RayDesc from GenerateCameraRay
            sReflectionPayload // payload
        );
        
        //sPayload.radiance = float3(1, 0, 1);
        sPayload.radiance = sReflectionPayload.radiance;
    }
    else
    {
        sPayload.radiance = DebugColorFromId(InstanceIndex());
    }
}