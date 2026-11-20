#include <h2/renderer/renderPipelines/realtimePathTracerRenderPipeline.h>

#include <array>
#include <string_view>

#include <d3d12.h>

#include <imgui.h>

#include <h2/renderer/framegraph/frameGraph.h>
#include <h2/renderer/gpu/scene/gpuScene.h>
#include <h2/shaders/shaderInterop.h>

namespace Hydrogen
{
    void RealtimePathTracerRenderPipeline::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler,
                                                      uint32 renderWidth, uint32 renderHeight)
    {
        m_pDevice = &device;

        m_gBufferPass.Initialize(device, shaderCompiler);
        m_buildTlasPass.Initialize(device, shaderCompiler);
        m_tonemapPass.Initialize(device, shaderCompiler);
        m_dummyPass.Initialize(device, shaderCompiler);
        m_restirInitialCandidateGenerationPass.Initialize(device, shaderCompiler);
        m_restirShadingPass.Initialize(device, shaderCompiler);
        m_accumulationPass.Initialize(device, shaderCompiler);
        m_accumulationPass.CreateAccumulationTarget(renderWidth, renderHeight);
    }

    void RealtimePathTracerRenderPipeline::Shutdown() {}

    void RealtimePathTracerRenderPipeline::DefineFrameGraphResources(FrameGraph& frameGraph,
                                                                     const FrameContext& frameContext)
    {
        frameGraph.CreateTexture("GBuffer_Albedo", {
                                                       .width = frameContext.renderWidth,
                                                       .height = frameContext.renderHeight,
                                                       .mipLevels = 1,
                                                       .arraySize = 1,
                                                       .format = GBufferPass::AlbedoFormat,
                                                       .flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                       .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                       .optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                                                   });

        frameGraph.CreateTexture("GBuffer_Normal", {
                                                       .width = frameContext.renderWidth,
                                                       .height = frameContext.renderHeight,
                                                       .mipLevels = 1,
                                                       .arraySize = 1,
                                                       .format = GBufferPass::NormalFormat,
                                                       .flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                       .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                       .optimizedClearColor = {0.0f, 0.0f},
                                                   });

        frameGraph.CreateTexture("GBuffer_RM", {
                                                   .width = frameContext.renderWidth,
                                                   .height = frameContext.renderHeight,
                                                   .mipLevels = 1,
                                                   .arraySize = 1,
                                                   .format = GBufferPass::RoughnessMetallicFormat,
                                                   .flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                   .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                   .optimizedClearColor = {0.0f, 0.0f},
                                               });

        // Combine with GBuffer_Normal?
        frameGraph.CreateTexture("GBuffer_MotionVectors", {
                                                              .width = frameContext.renderWidth,
                                                              .height = frameContext.renderHeight,
                                                              .mipLevels = 1,
                                                              .arraySize = 1,
                                                              .format = GBufferPass::MotionFormat,
                                                              .flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                              .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                              .optimizedClearColor = {0.0f, 0.0f},
                                                          });

        frameGraph.CreateTexture("SceneColor", {
                                                   .width = frameContext.renderWidth,
                                                   .height = frameContext.renderHeight,
                                                   .mipLevels = 1,
                                                   .arraySize = 1,
                                                   .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
                                                   .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                                                   .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                   .optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
                                               });

        frameGraph.ImportTexture("AccumulationTarget", m_accumulationPass.GetAccumulationTarget());

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

        frameGraph.CreateTexture("SceneDepth", {
                                                   .width = frameContext.renderWidth,
                                                   .height = frameContext.renderHeight,
                                                   .mipLevels = 1,
                                                   .arraySize = 1,
                                                   .format = GBufferPass::DepthFormat,
                                                   .flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                   .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                   .optimizedDepthClearValue = 0.0f,
                                               });

        const uint32 instanceCount = static_cast<uint32>(frameContext.renderScene.objects.size());
        const AccelerationStructureSizes tlasSizes = m_pDevice->GetTlasPrebuildSizes(instanceCount);

        frameGraph.CreateBuffer("TLAS", Buffer::Desc{.elementCount = static_cast<uint32>(tlasSizes.resultSize),
                                                     .stride = 1,
                                                     .flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE});

        frameGraph.CreateBuffer("TLASScratch", Buffer::Desc{.elementCount = static_cast<uint32>(tlasSizes.scratchSize),
                                                            .stride = 1,
                                                            .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS});

        frameGraph.CreateBuffer("ReSTIR_InitialReservoirs",
                                Buffer::Desc{.elementCount = frameContext.renderWidth * frameContext.renderHeight,
                                             .stride = sizeof(Reservoir),
                                             .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS});
    }

    std::string_view RealtimePathTracerRenderPipeline::Render(FrameGraph& frameGraph, const FrameContext& frameContext)
    {
        DefineFrameGraphResources(frameGraph, frameContext);

        m_buildTlasPass.pScene = &frameContext.gpuScene;
        frameGraph.AddPass("BuildTLAS", m_buildTlasPass);

        m_gBufferPass.pScene = &frameContext.gpuScene;
        m_gBufferPass.renderObjects = frameContext.renderScene.objects;
        frameGraph.AddPass("GBuffer", m_gBufferPass);

        frameGraph.AddPass("ReSTIR_CandidatesGeneration", m_restirInitialCandidateGenerationPass,
                           FGPassFlags::NeverCull);

        if (m_showGBuffer)
        {
            m_dummyPass.output = "Output";
            m_dummyPass.indexToOutput = static_cast<uint32>(m_gBufferView);
            frameGraph.AddPass("Dummy", m_dummyPass);
        }
        else
        {
            frameGraph.AddPass("ReSTIR_Shading", m_restirShadingPass);

            if (m_accumulate)
            {
                m_accumulationPass.frameNumber = frameContext.frameNumber;
                m_accumulationPass.resetAccumulation = frameContext.sceneChanged;
                frameGraph.AddPass("Accumulation", m_accumulationPass);
            }

            frameGraph.AddPass("Tonemap", m_tonemapPass);
        }

        return "Output";
    }

    void RealtimePathTracerRenderPipeline::BuildUI()
    {
        static constexpr std::array gBufferViewNames = {"Albedo", "Normal", "Roughness", "Metallic", "Depth"};
        static_assert(gBufferViewNames.size() == static_cast<size_t>(eGBufferView::Count),
                      "gBufferViewNames must list every eGBufferView");

        ImGui::Checkbox("Accumulate", &m_accumulate);
        if (m_accumulate)
        {
            ImGui::Text("Accumulated frames: %u", m_accumulationPass.GetAccumulatedFramesCount());
        }

        ImGui::Checkbox("Show G-buffer", &m_showGBuffer);

        if (m_showGBuffer)
        {
            auto selected = static_cast<int32>(m_gBufferView);
            if (ImGui::Combo("G-buffer view", &selected, gBufferViewNames.data(),
                             static_cast<int32>(gBufferViewNames.size())))
            {
                m_gBufferView = static_cast<eGBufferView>(selected);
            }
        }
    }
} // namespace Hydrogen
