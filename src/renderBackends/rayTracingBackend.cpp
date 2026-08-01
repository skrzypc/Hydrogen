#include "renderBackends/rayTracingBackend.h"

#include <string_view>

#include <d3d12.h>

#include "frameGraph.h"
#include "gpuScene.h"
#include "shaderInterop.h"

namespace Hydrogen
{
    void RayTracingBackend::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
    {
        m_pDevice = &device;

        m_buildTlasPass.Initialize(device, shaderCompiler);
        m_rayTraceDispatchPass.Initialize(device, shaderCompiler);
    }

    void RayTracingBackend::Shutdown()
    {

    }

    std::string_view RayTracingBackend::Render(FrameGraph& frameGraph, const FrameContext& frameContext)
    {
        DefineFrameGraphResources(frameGraph, frameContext);

        m_buildTlasPass.pScene = &frameContext.gpuScene;
        frameGraph.AddPass("BuildTLAS", m_buildTlasPass);

        m_rayTraceDispatchPass.outputTarget = "SceneColor";
        frameGraph.AddPass("RayTraceDispatch", m_rayTraceDispatchPass);

        return "SceneColor";
    }

    void RayTracingBackend::DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext)
    {
        frameGraph.CreateTexture("SceneColor",
            {
                .width = frameContext.renderWidth,
                .height = frameContext.renderHeight,
                .mipLevels = 1,
                .arraySize = 1,
                .format = frameContext.displayFormat,
                .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .optimizedClearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
            });

        const uint32 instanceCount = static_cast<uint32>(frameContext.renderScene.objects.size());
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
