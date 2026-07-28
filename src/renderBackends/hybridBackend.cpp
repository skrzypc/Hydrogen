#include "renderBackends/hybridBackend.h"

#include <string_view>

#include <d3d12.h>

#include "frameGraph.h"
#include "gpuScene.h"
#include "shaderInterop.h"

namespace Hydrogen
{
    void HybridBackend::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene)
    {
        m_pDevice = &device;
        m_pGpuScene = &gpuScene;
        m_clearPass.Initialize(device, shaderCompiler);
        m_clearPass.clearColor = { 0.53f, 0.81f, 0.92f, 1.0f };
        m_meshPass.Initialize(device, shaderCompiler);
        m_buildTlasPass.Initialize(device, shaderCompiler);
    }

    void HybridBackend::Shutdown()
    {

    }

    std::string_view HybridBackend::Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc)
    {
        DefineFrameGraphResources(frameGraph, scene, outputDesc);

        m_clearPass.target = "SceneColor";
        frameGraph.AddPass("ClearTarget", m_clearPass);

        m_buildTlasPass.pScene = m_pGpuScene;
        m_buildTlasPass.renderObjects = scene.objects;
        frameGraph.AddPass("BuildTLAS", m_buildTlasPass);

        m_meshPass.renderTarget = "SceneColor";
        m_meshPass.depthTarget = "SceneDepth";
        m_meshPass.pScene = m_pGpuScene;
        m_meshPass.renderObjects = scene.objects;
        frameGraph.AddPass("MeshPass", m_meshPass);

        return "SceneColor";
    }

    void HybridBackend::DefineFrameGraphResources(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc)
    {
        frameGraph.CreateTexture("SceneColor",
            {
                .width = outputDesc.width,
                .height = outputDesc.height,
                .mipLevels = 1,
                .arraySize = 1,
                .format = outputDesc.format,
                .flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .optimizedClearColor = m_clearPass.clearColor,
            });

        frameGraph.CreateTexture("SceneDepth",
            {
                .width = outputDesc.width,
                .height = outputDesc.height,
                .mipLevels = 1,
                .arraySize = 1,
                .format = DXGI_FORMAT_D32_FLOAT,
                .flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                .optimizedDepthClearValue = 0.0f,
            });

        const uint32 instanceCount = static_cast<uint32>(scene.objects.size());
        const BuildTlasPass::TlasSizes tlasSizes = m_buildTlasPass.QuerySizes(instanceCount);

        m_buildTlasPass.m_tlasHandle = frameGraph.CreateBuffer("TLAS",
            Buffer::Desc{ .size = tlasSizes.resultSize, .flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE });

        m_buildTlasPass.m_scratchHandle = frameGraph.CreateBuffer("TLASScratch",
            Buffer::Desc{ .size = tlasSizes.scratchSize, .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS });
    }

    void HybridBackend::FillFrameData(FrameData& frameData)
    {
        frameData.tlasIndex = m_buildTlasPass.GetTlasSrvIndex();
    }

    void HybridBackend::BuildUI()
    {
        
    }
}
