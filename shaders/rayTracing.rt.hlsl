#include "common.hlsli"
#include "rng.hlsli"

struct PushConstants
{
    uint tlasIndex;
    uint outputUavIndex;
    uint accumulationTargetUavIndex;
    uint accumulatedFramesCount;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct [raypayload] RayPayload
{
    float3 radiance : write(closesthit, miss) : read(caller);
    bool bounce : write(caller) : read(closesthit);
};

struct SurfaceHit
{
    float3 position;
    float3 normal;
    float2 uv;
    
    uint materialIndex;
};

float3 DebugColorFromId(uint id)
{
    uint hash = id * 2654435761u;
    return float3(
        float((hash >> 0) & 0xFF) / 255.0f,
        float((hash >> 8) & 0xFF) / 255.0f,
        float((hash >> 16) & 0xFF) / 255.0f);
}

SurfaceHit GetSurfaceHit(BuiltInTriangleIntersectionAttributes triangleAttributes)
{
    StructuredBuffer<GpuInstanceData> instancesDataBuffer = ResourceDescriptorHeap[g_frame.instanceDataBufferIndex];
    StructuredBuffer<GpuMeshData> meshesDataBuffer = ResourceDescriptorHeap[g_frame.meshDataBufferIndex];
    
    StructuredBuffer<uint> indicesBuffer = ResourceDescriptorHeap[g_frame.indexBufferIndex];
    
    StructuredBuffer<float3> positionsBuffer = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    StructuredBuffer<float3> normalsBuffer = ResourceDescriptorHeap[g_frame.vertexNormalBufferIndex];
    StructuredBuffer<float2> uvsBuffer = ResourceDescriptorHeap[g_frame.vertexUvBufferIndex];
    
    GpuInstanceData sInstanceData = instancesDataBuffer[InstanceID()];
    GpuMeshData sMeshData = meshesDataBuffer[sInstanceData.meshDataIndex];
    
    const uint firstIndex = sMeshData.baseIndex + PrimitiveIndex() * 3;
    
    const uint i0 = sMeshData.baseVertex + indicesBuffer[firstIndex + 0];
    const uint i1 = sMeshData.baseVertex + indicesBuffer[firstIndex + 1];
    const uint i2 = sMeshData.baseVertex + indicesBuffer[firstIndex + 2];

    const float b1 = triangleAttributes.barycentrics.x;
    const float b2 = triangleAttributes.barycentrics.y;
    const float b0 = 1.0f - b1 - b2;
    
    SurfaceHit hit;
    hit.position = b0 * positionsBuffer[i0] + b1 * positionsBuffer[i1] + b2 * positionsBuffer[i2];
    hit.normal = normalize(b0 * normalsBuffer[i0] + b1 * normalsBuffer[i1] + b2 * normalsBuffer[i2]);
    hit.uv = b0 * uvsBuffer[i0] + b1 * uvsBuffer[i1] + b2 * uvsBuffer[i2];
    
    hit.materialIndex = sInstanceData.materialDataIndex;
    
    return hit;
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
    float2 pixelCoords = float2(DispatchRaysIndex().xy);
    const float2 resolution = float2(DispatchRaysDimensions().xy);
    
    RngState rngState = InitRng(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, g_frame.frameNumber);
    
    const float2 pixelOffset = float2(NextRandomFloat(rngState), NextRandomFloat(rngState));
    pixelCoords += lerp(-0.5f.xx, 0.5f.xx, pixelOffset);
    
    pixelCoords = (((pixelCoords + 0.5f) / resolution) * 2.0f - 1.0f);
    
    RayDesc sWsRay = GenerateCameraRay(pixelCoords);
    
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
    RWTexture2D<float4> accumulationBuffer = ResourceDescriptorHeap[g_push.accumulationTargetUavIndex];
    
    float3 previousRadiance = (g_push.accumulatedFramesCount > 1u) ? accumulationBuffer[DispatchRaysIndex().xy].rgb : 0.0f.xxx;
    
    float3 accumulatedRadiance = previousRadiance + sPayload.radiance;
    accumulationBuffer[DispatchRaysIndex().xy] = float4(accumulatedRadiance, 1.0f);
    
    output[DispatchRaysIndex().xy] = float4(accumulatedRadiance / g_push.accumulatedFramesCount, 1.0f);
}

[shader("miss")]
void mainMiss(inout RayPayload sPayload)
{
    sPayload.radiance = float3(0.0f, 0.0f, 0.0f);
}

[shader("closesthit")]
void mainClosestHit(inout RayPayload sPayload, in BuiltInTriangleIntersectionAttributes attrs)
{
    SurfaceHit hit = GetSurfaceHit(attrs);
    
    const float3 worldPosition = mul(ObjectToWorld3x4(), float4(hit.position, 1.0f));
    const float3 worldNormal = normalize(mul(transpose((float3x3) WorldToObject3x4()), hit.normal)); // remove translation and scale from the normal
    
    StructuredBuffer<GpuMaterialData> materialDataBuffer = ResourceDescriptorHeap[g_frame.materialDataBufferIndex];
    GpuMaterialData sMaterialData = materialDataBuffer[hit.materialIndex];
    
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.5f));
    
    sPayload.radiance = sMaterialData.baseColor * dot(worldNormal, lightDir);
    
    //if (InstanceIndex() == 0 && sPayload.bounce)
    //if (InstanceIndex() % 2 == 0 && sPayload.bounce)
    //{
    //    RayPayload sReflectionPayload;
    //    sReflectionPayload.bounce = false;
        
    //    RngState rngState = InitRng(uint2(DispatchRaysIndex().xy), DispatchRaysDimensions().xy, g_frame.frameNumber);
        
    //    // cosine weighted hemisphere sampling
    //    float u1 = 0.5f; // NextRandomFloat(rngState);
    //    float u2 = 0.5f; // NextRandomFloat(rngState);
        
    //    float r = sqrt(u1);
    //    float phi = 2.0f * 3.14159265359f * u2;
        
    //    float x, y, z;
    //    sincos(phi, z, x);
    //    x *= r;
    //    z *= r;
    //    y = sqrt(max(0.0f, 1.0f - u1));
        
    //    float3 normal = float3(0.0f, 1.0f, 0.0f);
    //    float3 tangent, bitangent;
    //    OrthonormalBasis(normal, tangent, bitangent);
        
    //    RayDesc reflectionRay;
    //    reflectionRay.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    //    //reflectionRay.Direction = normalize(x * tangent + y * normal + z * bitangent); //reflect(WorldRayDirection(), float3(0.0f, 1.0f, 0.0f));
    //    reflectionRay.Direction = reflect(WorldRayDirection(), worldNormal);
    //    reflectionRay.TMin = 0.01f;
    //    reflectionRay.TMax = 1000000.0f;
        
    //    RaytracingAccelerationStructure sTlas = ResourceDescriptorHeap[g_push.tlasIndex];
        
    //    TraceRay(
    //        sTlas, // tlas
    //        RAY_FLAG_FORCE_OPAQUE, // flags
    //        0xFF, // instance mask
    //        0, // hit group offset (contributionToHitGroupIndex)
    //        1, // geometry multiplier (stride, usually 1 hit group per geometry)
    //        0, // miss shader index
    //        reflectionRay, // the RayDesc from GenerateCameraRay
    //        sReflectionPayload // payload
    //    );
        
    //    //sPayload.radiance = float3(1, 0, 1);
    //    sPayload.radiance = sReflectionPayload.radiance;
    //}
    //else
    //{
    //    sPayload.radiance = DebugColorFromId(InstanceIndex());
    //}
}