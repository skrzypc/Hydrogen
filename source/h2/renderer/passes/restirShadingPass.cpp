#include <directx/d3d12.h>

#include <h2/renderer/gpu/device.h>
#include <h2/shaders/shaderCompiler.h>
#include <h2/renderer/gpu/commands/graphicsContext.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>
#include <h2/renderer/passes/restirShadingPass.h>

namespace Hydrogen
{
    void ReSTIRShadingPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
    {
        m_pDevice = &device;

        Shader::Desc csDesc{
            .sourcePath = "restirShading.cs.hlsl",
            .name = "ReSTIRShading",
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

    void ReSTIRShadingPass::Setup(FGBuilder& builder)
    {
        builder.Read("TLAS", FGAccess::Read::AccelerationStructure);

        builder.Read("GBuffer_Albedo", FGAccess::Read::ShaderResource);
        builder.Read("GBuffer_Normal", FGAccess::Read::ShaderResource);
        builder.Read("GBuffer_RM", FGAccess::Read::ShaderResource);
        builder.Read("SceneDepth", FGAccess::Read::ShaderResource);

        builder.Read(reservoirs, FGAccess::Read::ShaderResource);

        builder.Write(output, FGAccess::Write::UnorderedAccess);

        const Texture::Desc& desc = builder.GetTextureDesc(output);
        m_width = desc.width;
        m_height = desc.height;
    }

    void ReSTIRShadingPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
    {
        ID3D12GraphicsCommandList10* cmd = graphicsContext.CmdList();

        cmd->SetComputeRootSignature(m_pDevice->GetRootSignature().Get());
        cmd->SetComputeRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer),
                                              GraphicsContext::s_frameDataAddr);
        cmd->SetPipelineState(m_pso.Get());

        const PushConstants push{
            .tlasIndex = fgExecuteContext.GetSRVIndex("TLAS"),
            .albedoIndex = fgExecuteContext.GetSRVIndex("GBuffer_Albedo"),
            .normalIndex = fgExecuteContext.GetSRVIndex("GBuffer_Normal"),
            .roughnessMetallicIndex = fgExecuteContext.GetSRVIndex("GBuffer_RM"),
            .depthIndex = fgExecuteContext.GetSRVIndex("SceneDepth"),
            .reservoirIndex = fgExecuteContext.GetSRVIndex(reservoirs),
            .outputIndex = fgExecuteContext.GetUAVIndex(output),
        };
        graphicsContext.SetComputePushConstants(push);

        const uint32 groupCountX = (m_width + ThreadGroupSize - 1) / ThreadGroupSize;
        const uint32 groupCountY = (m_height + ThreadGroupSize - 1) / ThreadGroupSize;
        cmd->Dispatch(groupCountX, groupCountY, 1);
    }
} // namespace Hydrogen
