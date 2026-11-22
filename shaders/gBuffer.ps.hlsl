#include "include/common.hlsli"
#include "include/shaderUtils.hlsli"

struct PushConstants
{
    uint transformIndex;
    uint materialIndex;
    uint baseVertex;
};

ConstantBuffer<PushConstants> g_push : register(b0, space0);

struct PsIn
{
    float4 positionClipSpace : SV_Position;
    float3 normalWorldSpace : NORMAL;
    float4 tangentWorldSpace : TANGENT;
    float2 uvCoords : TEXCOORD0;
};

struct PsOut
{
    float4 albedo : SV_Target0;
    float4 normals : SV_Target1;
    float2 roughnessMetallic : SV_Target2;
};

PsOut mainPS(PsIn input)
{
    StructuredBuffer<GpuMaterialData> materialDataBuffer = ResourceDescriptorHeap[g_frame.materialDataBufferIndex];

    GpuMaterialData material = materialDataBuffer[g_push.materialIndex];

    PsOut output;

    float3 albedo = material.albedo;
    if (material.albedoTextureIndex != InvalidTextureIndex)
    {
        Texture2D<float4> albedoTexture = ResourceDescriptorHeap[material.albedoTextureIndex];
        albedo *= albedoTexture.Sample(AnisoWrapSampler, input.uvCoords).rgb;
    }

    float2 roughnessMetallic = float2(material.roughness, material.metallic);
    if (material.roughnessMetallicTextureIndex != InvalidTextureIndex)
    {
        Texture2D<float4> roughnessMetallicTexture = ResourceDescriptorHeap[material.roughnessMetallicTextureIndex];
        roughnessMetallic = roughnessMetallicTexture.Sample(AnisoWrapSampler, input.uvCoords).gb;
    }

    float3 shadingNormal = normalize(input.normalWorldSpace);

    // Orient the geometric normal toward the camera, then make the shading normal agree with it.
    StructuredBuffer<ViewData> views = ResourceDescriptorHeap[g_frame.viewBufferIndex];
    ViewData view = views[g_frame.mainViewIndex];
    float3 worldPosition = ReconstructWorldPosition(input.positionClipSpace.xy / view.viewportSize,
                                                    input.positionClipSpace.z, view.invViewProjectionMx);
    float3 geometryNormal = normalize(cross(ddx(worldPosition), ddy(worldPosition)));
    geometryNormal *= dot(geometryNormal, view.worldPosition - worldPosition) < 0.0f ? -1.0f : 1.0f;
    shadingNormal *= dot(geometryNormal, shadingNormal) < 0.0f ? -1.0f : 1.0f;

    if (material.normalTextureIndex != InvalidTextureIndex)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[material.normalTextureIndex];
        float3 tangentSpaceNormal = normalTexture.Sample(AnisoWrapSampler, input.uvCoords).xyz * 2.0f - 1.0f;

        float3 T = normalize(input.tangentWorldSpace.xyz);
        float3 N = shadingNormal;
        T = normalize(T - N * dot(N, T)); // re-orthogonalize after interpolation
        float3 B = cross(N, T) * input.tangentWorldSpace.w;
        float3x3 TBN = float3x3(T, B, N);

        shadingNormal = normalize(mul(tangentSpaceNormal, TBN));
    }

    output.albedo = float4(albedo, 1.0f);
    // TODO: We only need it for RT. Skip this for rasterization.
    output.normals = float4(EncodeOctahedral(shadingNormal), EncodeOctahedral(geometryNormal));
    output.roughnessMetallic = float2(roughnessMetallic.x, roughnessMetallic.y);

    return output;
}
