#include "renderBackends/rayTracingBackend.h"

#include <string_view>

#include <d3d12.h>

#include "frameGraph.h"
#include "gpuScene.h"
#include "shaderInterop.h"

namespace Hydrogen
{
    void RayTracingBackend::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene)
    {
        m_pDevice = &device;
        m_pGpuScene = &gpuScene;

        m_buildTlasPass.Initialize(device, shaderCompiler);
        m_rayTraceDispatchPass.Initialize(device, shaderCompiler);
    }

    void RayTracingBackend::Shutdown()
    {

    }

    std::string_view RayTracingBackend::Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc)
    {
        DefineFrameGraphResources(frameGraph, scene, outputDesc);

        m_buildTlasPass.pScene = m_pGpuScene;
        frameGraph.AddPass("BuildTLAS", m_buildTlasPass);

        m_rayTraceDispatchPass.outputTarget = "SceneColor";
        frameGraph.AddPass("RayTraceDispatch", m_rayTraceDispatchPass);

        return "SceneColor";
    }

    void RayTracingBackend::DefineFrameGraphResources(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc)
    {
        frameGraph.CreateTexture("SceneColor",
            {
                .width = outputDesc.width,
                .height = outputDesc.height,
                .mipLevels = 1,
                .arraySize = 1,
                .format = outputDesc.format,
                .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .optimizedClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            });

        const uint32 instanceCount = static_cast<uint32>(scene.objects.size());
        const AccelerationStructureSizes tlasSizes = m_pDevice->GetTlasPrebuildSizes(instanceCount);

        frameGraph.CreateBuffer("TLAS",
            Buffer::Desc{ .size = tlasSizes.resultSize, .flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE });

        frameGraph.CreateBuffer("TLASScratch",
            Buffer::Desc{ .size = tlasSizes.scratchSize, .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS });
    }

    void RayTracingBackend::BuildUI()
    {

    }
}
