#include <directx/d3d12.h>

#include <h2/renderer/gpu/device.h>
#include <h2/shaders/shaderCompiler.h>
#include <h2/renderer/gpu/commands/graphicsContext.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>
#include <h2/renderer/passes/debug/accumulationPass.h>

namespace Hydrogen
{
    void AccumulationPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
    {
        m_pDevice = &device;

        Shader::Desc csDesc{
            .sourcePath = "accumulation.cs.hlsl",
            .name = "AccumulationCS",
            .entryPoint = "mainCS",
            .type = eShaderType::CS,
        };

        Shader cs(csDesc);
        shaderCompiler.Compile(cs);

        PipelineState::ComputeDesc psoDesc{
            .pComputeShader = &cs,
        };

        m_pso.CreateCompute(device, psoDesc);
    }

    void AccumulationPass::CreateAccumulationTarget(uint32 width, uint32 height)
    {
        const Texture::Desc desc{
            .width = width,
            .height = height,
            .mipLevels = 1,
            .arraySize = 1,
            .format = AccumulationFormat,
            .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        };

        // A defined layout: the frame graph returns imported textures to it after every frame, and a barrier
        // from UNDEFINED would not guarantee that the accumulated sum survives.
        m_pAccumulationTarget = m_pDevice->CreateTexture(L"DebugAccumulationTarget", desc,
                                                         ResourceState{.layout = D3D12_BARRIER_LAYOUT_COMMON});
        m_accumulatedFramesCount = 0;
    }

    void AccumulationPass::Setup(FGBuilder& builder)
    {
        // Read and written in place through a UAV: each thread only touches its own pixel.
        builder.Write(sceneColor, FGAccess::Write::UnorderedAccess);
        builder.Write(accumulationTarget, FGAccess::Write::UnorderedAccess);

        const Texture::Desc& desc = builder.GetTextureDesc(sceneColor);
        m_width = desc.width;
        m_height = desc.height;

        const bool skippedFrame = !m_lastFrameNumber.has_value() || frameNumber != *m_lastFrameNumber + 1;
        m_accumulatedFramesCount = (resetAccumulation || skippedFrame) ? 1 : m_accumulatedFramesCount + 1;
        m_lastFrameNumber = frameNumber;
    }

    void AccumulationPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
    {
        ID3D12GraphicsCommandList10* cmd = graphicsContext.CmdList();

        cmd->SetComputeRootSignature(m_pDevice->GetRootSignature().Get());
        cmd->SetComputeRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer),
                                              GraphicsContext::s_frameDataAddr);
        cmd->SetPipelineState(m_pso.Get());

        const PushConstants push{
            .sceneColorIndex = fgExecuteContext.GetUAVIndex(sceneColor),
            .accumulationTargetIndex = fgExecuteContext.GetUAVIndex(accumulationTarget),
            .accumulatedFramesCount = m_accumulatedFramesCount,
        };
        graphicsContext.SetComputePushConstants(push);

        const uint32 groupCountX = (m_width + ThreadGroupSize - 1) / ThreadGroupSize;
        const uint32 groupCountY = (m_height + ThreadGroupSize - 1) / ThreadGroupSize;
        cmd->Dispatch(groupCountX, groupCountY, 1);
    }
} // namespace Hydrogen
