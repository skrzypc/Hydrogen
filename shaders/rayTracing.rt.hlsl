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

static const uint MIN_BOUNCES = 3;
static const uint MAX_BOUNCES = 256;

struct [raypayload] RayPayload
{
    float3 shadingNormal : write(closesthit) : read(caller);
    float3 geometryNormal : write(closesthit) : read(caller);
    float2 uv : write(closesthit) : read(caller);
    float hitDistance : write(closesthit, miss) : read(caller);
    uint materialId : write(closesthit) : read(caller);
};

struct SurfaceHit
{
    float3 position;
    float3 shadingNormal;
    float3 geometryNormal;
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
    
    const float3 v0 = mul(ObjectToWorld3x4(), float4(positionsBuffer[i0], 1.0f)).xyz;
    const float3 v1 = mul(ObjectToWorld3x4(), float4(positionsBuffer[i1], 1.0f)).xyz;
    const float3 v2 = mul(ObjectToWorld3x4(), float4(positionsBuffer[i2], 1.0f)).xyz;
    
    SurfaceHit hit;
    hit.position = b0 * v0 + b1 * v1 + b2 * v2;
    hit.shadingNormal = normalize(mul(transpose((float3x3)WorldToObject3x4()), b0 * normalsBuffer[i0] + b1 * normalsBuffer[i1] + b2 * normalsBuffer[i2]));
    hit.geometryNormal = normalize(cross(v1 - v0, v2 - v0));
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

float CalculateLuminance(const float3 input)
{
    return (0.2126f * input.r + 0.7152f * input.g + 0.0722f * input.b);
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
    
    // Primary ray
    RayDesc currentRay = GenerateCameraRay(pixelCoords);
    
    RayPayload rayPayload;
    float3 currentRadiance = 0.0f.xxx;
    float3 throughput = 1.0f.xxx;
    
    RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[g_push.tlasIndex];
    for (uint i = 0; i <= MAX_BOUNCES; ++i)
    {
        TraceRay(
            tlas,
            RAY_FLAG_FORCE_OPAQUE, // flags
            0xFF, // instance mask
            0, // hit group offset (contributionToHitGroupIndex)
            1, // geometry multiplier (stride, usually 1 hit group per geometry)
            0, // miss shader index
            currentRay, // the RayDesc from GenerateCameraRay
            rayPayload // payload
        );
        
        bool hit = rayPayload.hitDistance > 0.0f;
        if (hit)
        {
            float3 hitWorldPosition = currentRay.Origin + currentRay.Direction * rayPayload.hitDistance;

            // Flip normal if necessary.
            rayPayload.geometryNormal *= dot(rayPayload.geometryNormal, -currentRay.Direction) < 0.0f ? -1.0f : 1.0f;
            rayPayload.shadingNormal *= dot(rayPayload.geometryNormal, rayPayload.shadingNormal) < 0.0f ? -1.0f : 1.0f;
            
            float3 tangent, bitangent;
            OrthonormalBasis(rayPayload.shadingNormal, tangent, bitangent);
            
            // cosine weighted hemisphere sampling
            float u1 = NextRandomFloat(rngState);
            float u2 = NextRandomFloat(rngState);
        
            float r = sqrt(u1);
            float phi = 2.0f * 3.14159265359f * u2;
        
            float x, y, z;
            sincos(phi, z, x);
            x *= r;
            z *= r;
            y = sqrt(max(0.0f, 1.0f - u1));
            
            float3 bounceRayDir = normalize(x * tangent + y * rayPayload.shadingNormal + z * bitangent);

            // brdf part
            throughput *= float3(0.9f, 0.9f, 0.9f);
            //throughput *= float3(1.0f, 1.0f, 1.0f);
            //
            
            if (i > MIN_BOUNCES)
            {
                float russianRuletteFactor = min(0.95f, CalculateLuminance(throughput));
                
                if (russianRuletteFactor < NextRandomFloat(rngState))
                {
                    break;
                }
                
                throughput /= russianRuletteFactor;
            }
            
            currentRay.Direction = bounceRayDir;
            currentRay.Origin = hitWorldPosition + rayPayload.geometryNormal * 0.001f; // TODO: Implement RT Gems 2, Chapter 6
            currentRay.TMin = 0.0f;
            currentRay.TMax = 100000.0f;
        }
        else
        {
            currentRadiance += throughput * float3(1.0f, 1.0f, 1.0f);
            break;
        }
    }
    
    RWTexture2D<float4> outputTarget = ResourceDescriptorHeap[g_push.outputUavIndex];
    RWTexture2D<float4> accumulationTarget = ResourceDescriptorHeap[g_push.accumulationTargetUavIndex];
    
    float3 previousRadiance = (g_push.accumulatedFramesCount > 1u) ? accumulationTarget[DispatchRaysIndex().xy].rgb : 0.0f.xxx;
    
    float3 accumulatedRadiance = previousRadiance + currentRadiance;
    accumulationTarget[DispatchRaysIndex().xy] = float4(accumulatedRadiance, 1.0f);
    
    outputTarget[DispatchRaysIndex().xy] = float4(accumulatedRadiance / g_push.accumulatedFramesCount, 1.0f);
}

[shader("miss")]
void mainMiss(inout RayPayload rayPayload)
{
    rayPayload.hitDistance = -1.0f;
}

[shader("closesthit")]
void mainClosestHit(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes triangleAttributes)
{
    SurfaceHit hit = GetSurfaceHit(triangleAttributes);
   
    rayPayload.shadingNormal = hit.shadingNormal;
    rayPayload.geometryNormal = hit.geometryNormal;
    rayPayload.uv = hit.uv;
    rayPayload.hitDistance = RayTCurrent();
    rayPayload.materialId = 0;
}
