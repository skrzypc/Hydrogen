#include <h2/renderer/renderPipelines/referencePathTracerRenderPipeline.h>

#include <string_view>

#include <directx/d3d12.h>

#include <imgui.h>

#include <h2/renderer/framegraph/frameGraph.h>
#include <h2/renderer/gpu/scene/gpuScene.h>
#include <h2/renderer/passes/lightingPass.h>
#include <h2/shaders/shaderInterop.h>

namespace Hydrogen
{
    void ReferencePathTracerRenderPipeline::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler,
                                                       uint32 renderWidth, uint32 renderHeight)
    {
        m_pDevice = &device;

        m_buildTlasPass.Initialize(device, shaderCompiler);
        m_rayTraceDispatchPass.Initialize(device, shaderCompiler);
        m_rayTraceDispatchPass.CreateAccumulationTarget(renderWidth, renderHeight);
        m_tonemapPass.Initialize(device, shaderCompiler);
    }

    void ReferencePathTracerRenderPipeline::Shutdown() {}

    std::string_view ReferencePathTracerRenderPipeline::Render(FrameGraph& frameGraph, const FrameContext& frameContext)
    {
        DefineFrameGraphResources(frameGraph, frameContext);

        m_buildTlasPass.pScene = &frameContext.gpuScene;
        frameGraph.AddPass("BuildTLAS", m_buildTlasPass);

        m_rayTraceDispatchPass.outputTarget = "SceneColor";
        m_rayTraceDispatchPass.resetAccumulation = frameContext.sceneChanged;
        frameGraph.AddPass("RayTraceDispatch", m_rayTraceDispatchPass);

        frameGraph.AddPass("Tonemap", m_tonemapPass);

        return "Output";
    }

    void ReferencePathTracerRenderPipeline::DefineFrameGraphResources(FrameGraph& frameGraph,
                                                                      const FrameContext& frameContext)
    {
        frameGraph.CreateTexture("SceneColor", {
                                                   .width = frameContext.renderWidth,
                                                   .height = frameContext.renderHeight,
                                                   .mipLevels = 1,
                                                   .arraySize = 1,
                                                   .format = LightingPass::SceneColorFormat,
                                                   .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                   .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                   .optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                                               });

        frameGraph.CreateTexture("Output", {
                                               .width = frameContext.displayWidth,
                                               .height = frameContext.displayHeight,
                                               .mipLevels = 1,
                                               .arraySize = 1,
                                               .format = frameContext.displayFormat,
                                               .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                               .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                               .optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                                           });

        frameGraph.ImportTexture("AccumulationTarget", m_rayTraceDispatchPass.GetAccumulationTarget());

        const uint32 instanceCount = static_cast<uint32>(frameContext.renderScene.objects.size());
        const AccelerationStructureSizes tlasSizes = m_pDevice->GetTlasPrebuildSizes(instanceCount);

        frameGraph.CreateBuffer("TLAS", Buffer::Desc{.elementCount = static_cast<uint32>(tlasSizes.resultSize),
                                                     .stride = 1,
                                                     .flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE});

        frameGraph.CreateBuffer("TLASScratch", Buffer::Desc{.elementCount = static_cast<uint32>(tlasSizes.scratchSize),
                                                            .stride = 1,
                                                            .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS});
    }

    void ReferencePathTracerRenderPipeline::BuildUI()
    {
        ImGui::Text("Accumulated frames: %u", m_rayTraceDispatchPass.GetAccumulatedFramesCount());
    }
} // namespace Hydrogen
