
#include "include/common.hlsli"
#include "include/lighting.hlsli"
#include "include/rng.hlsli"
#include "include/shaderUtils.hlsli"
#include "include/brdf.hlsli"

struct PushConstants
{
    uint tlasIndex;
    uint outputUavIndex;
    uint accumulationTargetUavIndex;
    uint accumulatedFramesCount;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

//static const float3 kSkyRadiance = float3(0.0f, 0.0f, 0.0f);
static const float3 kSkyRadiance = float3(0.02f, 0.04f, 0.08f);
// static const float3 kSkyRadiance = float3(0.05f, 0.05f, 0.05f);

static const uint MIN_BOUNCES = 1;
static const uint MAX_BOUNCES = 3;

struct [raypayload] RayPayload
{
    float3 shadingNormal : write(closesthit) : read(caller);
float4 shadingTangent : write(closesthit) : read(caller);
float3 geometryNormal : write(closesthit) : read(caller);
float2 uv : write(closesthit) : read(caller);
float hitDistance : write(closesthit, miss) : read(caller);
uint materialId : write(closesthit) : read(caller);
};

struct [raypayload] ShadowRayPayload
{
    bool occluded : write(caller, miss) : read(caller);
};

struct SurfaceHit
{
    float3 position;
    float3 geometryNormal;
    float3 shadingNormal;
    float4 shadingTangent;
    float2 uv;

    uint materialIndex;
};

SurfaceHit GetSurfaceHit(BuiltInTriangleIntersectionAttributes triangleAttributes)
{
    StructuredBuffer<GpuInstanceData> instancesDataBuffer = ResourceDescriptorHeap[g_frame.instanceDataBufferIndex];
    StructuredBuffer<GpuMeshData> meshesDataBuffer = ResourceDescriptorHeap[g_frame.meshDataBufferIndex];

    StructuredBuffer<uint> indicesBuffer = ResourceDescriptorHeap[g_frame.indexBufferIndex];

    StructuredBuffer<float3> positionsBuffer = ResourceDescriptorHeap[g_frame.vertexPositionBufferIndex];
    StructuredBuffer<float3> normalsBuffer = ResourceDescriptorHeap[g_frame.vertexNormalBufferIndex];
    StructuredBuffer<float4> tangentsBuffer = ResourceDescriptorHeap[g_frame.vertexTangentBufferIndex];
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

    const float4 t0 = tangentsBuffer[i0];
    const float4 t1 = tangentsBuffer[i1];
    const float4 t2 = tangentsBuffer[i2];

    SurfaceHit hit;
    hit.position = b0 * v0 + b1 * v1 + b2 * v2;
    hit.shadingNormal = normalize(mul(transpose((float3x3) WorldToObject3x4()),
                                      b0 * normalsBuffer[i0] + b1 * normalsBuffer[i1] + b2 * normalsBuffer[i2]));
    hit.geometryNormal = normalize(cross(v1 - v0, v2 - v0));
    hit.shadingTangent.xyz = normalize(mul((float3x3) ObjectToWorld3x4(), b0 * t0.xyz + b1 * t1.xyz + b2 * t2.xyz));
    hit.shadingTangent.w = b0 * t0.w + b1 * t1.w + b2 * t2.w;
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
    wsRay.Direction = normalize((vfPixel.x * view.viewMx[0].xyz * fTanHalfFovY * fAspectRatio) -
                                (vfPixel.y * view.viewMx[1].xyz * fTanHalfFovY) + view.viewMx[2].xyz);

    return wsRay;
}

// TODO: Implement RT Gems 2, Chapter 6's robust self-intersection avoidance instead of a fixed epsilon.
static const float kRayOriginBiasDistance = 0.001f;

float3 OffsetRayOrigin(const float3 worldPosition, const float3 geometryNormal)
{
    return worldPosition + geometryNormal * kRayOriginBiasDistance;
}

[shader("raygeneration")]
void mainRayGen()
{
    float2 pixelCoords = float2(DispatchRaysIndex().xy);
    const float2 resolution = float2(DispatchRaysDimensions().xy);

    RngState rngState = InitRng(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, g_frame.frameNumber);

    // Anti-alliasing
    const float2 pixelOffset = float2(NextRandomFloat(rngState), NextRandomFloat(rngState));
    pixelCoords += lerp(-0.5f.xx, 0.5f.xx, pixelOffset);

    // To [-1.0, 1.0]
    pixelCoords = (((pixelCoords + float2(0.5f, 0.5f)) / resolution) * 2.0f - 1.0f);

    // Primary ray
    RayDesc currentRay = GenerateCameraRay(pixelCoords);

    RayPayload rayPayload;

    // The actual output.
    // Sum of every light contribution (NEE + emissive) collected so far, each already weighted by throughput at the
    // time.
    float3 currentRadiance = 0.0f.xxx;
    // Cumulative BRDF * cosTheta / pdf product along the path so far.
    // How much of any future contribution along this path still reaches the camera.
    float3 throughput = 1.0f.xxx;

    RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[g_push.tlasIndex];
    StructuredBuffer<GpuMaterialData> materialDataBuffer = ResourceDescriptorHeap[g_frame.materialDataBufferIndex];

    for (uint i = 0; i <= MAX_BOUNCES; ++i)
    {
        // i = 0 <- Primary ray.
        // i > 0 <- Bounce ray.
        TraceRay(tlas,
            RAY_FLAG_FORCE_OPAQUE, // flags
            0xFF, // instance mask
            0, // hit group offset (contributionToHitGroupIndex)
            1, // geometry multiplier (stride, usually 1 hit group per geometry)
            0, // miss shader index
            currentRay, rayPayload
        );

        bool hit = rayPayload.hitDistance > 0.0f;

        // Miss, finish the loop.
        if (!hit)
        {
            currentRadiance += throughput * kSkyRadiance;
            break;
        }

        // Evaluate hit position and hit normals
        float3 hitWorldPosition = currentRay.Origin + currentRay.Direction * rayPayload.hitDistance;

        // Flip normal if necessary.
        rayPayload.geometryNormal *= dot(rayPayload.geometryNormal, -currentRay.Direction) < 0.0f ? -1.0f : 1.0f;
        rayPayload.shadingNormal *= dot(rayPayload.geometryNormal, rayPayload.shadingNormal) < 0.0f ? -1.0f : 1.0f;

        GpuMaterialData material = materialDataBuffer[rayPayload.materialId];

        if (material.normalTextureIndex != InvalidTextureIndex)
        {
            Texture2D<float4> normalTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.normalTextureIndex)];
            float3 tangentSpaceNormal = normalTexture.SampleLevel(AnisoWrapSampler, rayPayload.uv, 0.0f).xyz * 2.0f - 1.0f;

            float3 T = normalize(rayPayload.shadingTangent.xyz);
            float3 N = rayPayload.shadingNormal;
            T = normalize(T - N * dot(N, T)); // re-orthogonalize after interpolation
            float3 B = cross(N, T) * rayPayload.shadingTangent.w;
            float3x3 TBN = float3x3(T, B, N);

            rayPayload.shadingNormal = normalize(mul(tangentSpaceNormal, TBN));
        }

        Surface surface;
        
        surface.albedo = material.albedo;
        if (material.albedoTextureIndex != InvalidTextureIndex)
        {
            Texture2D<float4> albedoTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.albedoTextureIndex)];
            // TODO: Mipmapping.
            surface.albedo *= albedoTexture.SampleLevel(AnisoWrapSampler, rayPayload.uv, 0.0f).rgb;
        }

        surface.roughness = material.roughness;
        surface.metallic = material.metallic;
        if (material.roughnessMetallicTextureIndex != InvalidTextureIndex)
        {
            Texture2D<float4> roughnessMetallicTexture = ResourceDescriptorHeap[NonUniformResourceIndex(material.roughnessMetallicTextureIndex)];
            float2 roughnessMetallic = roughnessMetallicTexture.SampleLevel(AnisoWrapSampler, rayPayload.uv, 0.0f).gb;

            surface.roughness = roughnessMetallic.x;
            surface.metallic = roughnessMetallic.y;
        }

        // Material emissive contribution.
        {
            currentRadiance += throughput * material.emissive;
        }

        // Light contribution.
        // Next Event Estimation. Sample a light, add its contribution if unoccluded.
        GpuLight sampledLight;
        float lightSampleWeight;
        if (SampleLightRIS(rngState, hitWorldPosition, rayPayload.shadingNormal, sampledLight, lightSampleWeight))
        {
            float3 lightDirection;
            float lightDistance;
            GetLightDirectionAndDistance(sampledLight, hitWorldPosition, lightDirection, lightDistance);

            // Check occlusion.
            RayDesc shadowRay;

            shadowRay.Origin = OffsetRayOrigin(hitWorldPosition, rayPayload.geometryNormal);
            shadowRay.Direction = lightDirection;
            shadowRay.TMin = 0.0f;
            shadowRay.TMax = lightDistance > 0.0f ? lightDistance * 0.999f : 100000.0f;

            ShadowRayPayload shadowRayPayload;
            shadowRayPayload.occluded = true;
            TraceRay(tlas,
                     RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER |
                         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                     0xFF, // instance mask
                     0, // hit group offset (contributionToHitGroupIndex)
                     1, // geometry multiplier (stride, usually 1 hit group per geometry)
                     1, // miss shader index
                     shadowRay, shadowRayPayload);

            // Light unoccluded, calculate total contribution for given point on surface.
            if (!shadowRayPayload.occluded)
            {
                float3 lightRadiance = GetLightContributionPT(sampledLight, lightDirection, lightDistance);
                //float3 materialBrdf = EvaluateBrdfLambert(surface); // Lambert
                float3 materialBrdf = EvaluateBrdfGgx(surface, rayPayload.shadingNormal, -currentRay.Direction, lightDirection); // GGX
                float NoL = saturate(dot(rayPayload.shadingNormal, lightDirection));

                currentRadiance += throughput * materialBrdf * lightRadiance * NoL * lightSampleWeight;
            }
        }

        // Russian Roulette
        // Probabilistically kill the path based on throughput, boosting survivors to compensate.
        // Unbiased alternative to a hard bounce cutoff.
        if (i > MIN_BOUNCES)
        {
            float russianRuletteFactor = min(0.95f, CalculateLuminance(throughput));

            if (russianRuletteFactor < NextRandomFloat(rngState))
            {
                break;
            }

            // Keeps result unbiased.
            throughput /= russianRuletteFactor;
        }

        // Generate bounce ray.
        
        // Lambert
        //BrdfSample brdfSample = SampleBrdfLambert(
        //    surface,
        //    rayPayload.shadingNormal,
        //    float2(NextRandomFloat(rngState), NextRandomFloat(rngState))
        //);
        
        // GGX
        BrdfSample brdfSample = SampleBrdfGgx(
            surface,
            rayPayload.shadingNormal,
            -currentRay.Direction,
            float2(NextRandomFloat(rngState), NextRandomFloat(rngState))
        );

        if (dot(brdfSample.direction, rayPayload.geometryNormal) <= 0.0f)
        {
            break;
        }
        
        throughput *= brdfSample.weight;
        
        currentRay.Origin = OffsetRayOrigin(hitWorldPosition, rayPayload.geometryNormal);
        currentRay.Direction = brdfSample.direction;
        currentRay.TMin = 0.0f;
        currentRay.TMax = 100000.0f;
    }

    RWTexture2D<float4> outputTarget = ResourceDescriptorHeap[g_push.outputUavIndex];
    RWTexture2D<float4> accumulationTarget = ResourceDescriptorHeap[g_push.accumulationTargetUavIndex];

    float3 previousRadiance =
        (g_push.accumulatedFramesCount > 1u) ? accumulationTarget[DispatchRaysIndex().xy].rgb : 0.0f.xxx;

    float3 accumulatedRadiance = previousRadiance + currentRadiance;
    accumulationTarget[DispatchRaysIndex().xy] = float4(accumulatedRadiance, 1.0f);

    outputTarget[DispatchRaysIndex().xy] = float4(accumulatedRadiance / g_push.accumulatedFramesCount, 1.0f);
}

[shader("miss")]
void mainMiss(inout RayPayload rayPayload)
{
    rayPayload.hitDistance = -1.0f;
}

[shader("miss")]
void shadowMiss(inout ShadowRayPayload shadowRayPayload)
{
    shadowRayPayload.occluded = false;
}

[shader("closesthit")]
void mainClosestHit(inout RayPayload rayPayload,
                     in BuiltInTriangleIntersectionAttributes triangleAttributes)
{
    SurfaceHit hit = GetSurfaceHit(triangleAttributes);

    rayPayload.shadingNormal = hit.shadingNormal;
    rayPayload.shadingTangent = hit.shadingTangent;
    rayPayload.geometryNormal = hit.geometryNormal;
    rayPayload.uv = hit.uv;
    rayPayload.hitDistance = RayTCurrent();
    rayPayload.materialId = hit.materialIndex;
}
