#include "renderBackends/rasterBackend.h"

#include <string_view>

#include <d3d12.h>

#include "frameGraph.h"
#include "gpuScene.h"

namespace Hydrogen
{
    void RasterBackend::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler, GpuScene& gpuScene)
    {
        m_pGpuScene = &gpuScene;
        m_clearPass.Initialize(device, shaderCompiler);
        m_clearPass.clearColor = { 0.53f, 0.81f, 0.92f, 1.0f };
        m_meshPass.Initialize(device, shaderCompiler);
    }

    void RasterBackend::Shutdown()
    {

    }

    std::string_view RasterBackend::Render(FrameGraph& frameGraph, const RenderScene& scene, const Texture::Desc& outputDesc)
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

        m_clearPass.target = "SceneColor";
        frameGraph.AddPass("ClearTarget", m_clearPass);

        m_meshPass.target = "SceneColor";
        m_meshPass.depthTarget = "SceneDepth";
        m_meshPass.pScene = m_pGpuScene;
        m_meshPass.renderObjects = scene.objects;
        frameGraph.AddPass("MeshPass", m_meshPass);

        return "SceneColor";
    }

    void RasterBackend::BuildUI()
    {
        
    }
}
