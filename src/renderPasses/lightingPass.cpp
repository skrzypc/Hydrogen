#include <d3d12.h>

#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "graphicsContext.h"
#include "renderPasses/lightingPass.h"

namespace Hydrogen
{
	void LightingPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		m_pDevice = &device;

		Shader::Desc csDesc
		{
			.sourcePath = "lighting.cs.hlsl",
			.name = "LightingCS",
			.entryPoint = "mainCS",
			.type = eShaderType::CS,
		};

		Shader cs(csDesc);
		shaderCompiler.Compile(cs);

		PipelineState::ComputeDesc psoDesc
		{
			.pComputeShader = &cs,
		};

		m_pso.CreateCompute(device, psoDesc);
	}

	void LightingPass::Setup(FGBuilder& builder)
	{
		builder.Read("GBuffer_Albedo", FGAccess::Read::ShaderResource);
		builder.Read("GBuffer_Normal", FGAccess::Read::ShaderResource);
		builder.Read("GBuffer_RM", FGAccess::Read::ShaderResource);
		builder.Read("SceneDepth", FGAccess::Read::ShaderResource);

		builder.Write("SceneColor", FGAccess::Write::UnorderedAccess);

		const Texture::Desc& desc = builder.GetTextureDesc("SceneColor");
		m_width = desc.width;
		m_height = desc.height;
	}

	void LightingPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		ID3D12GraphicsCommandList10* cmd = graphicsContext.CmdList();

		cmd->SetComputeRootSignature(m_pDevice->GetRootSignature().Get());
		cmd->SetComputeRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer), GraphicsContext::s_frameDataAddr);
		cmd->SetPipelineState(m_pso.Get());

		const PushConstants push
		{
			.albedoIndex = fgExecuteContext.GetSRVIndex("GBuffer_Albedo"),
			.normalIndex = fgExecuteContext.GetSRVIndex("GBuffer_Normal"),
			.roughnessMetalnessIndex = fgExecuteContext.GetSRVIndex("GBuffer_RM"),
			.depthIndex = fgExecuteContext.GetSRVIndex("SceneDepth"),
			.outputIndex = fgExecuteContext.GetUAVIndex("SceneColor"),
		};
		graphicsContext.SetComputePushConstants(push);

		const uint32 groupCountX = (m_width + ThreadGroupSize - 1) / ThreadGroupSize;
		const uint32 groupCountY = (m_height + ThreadGroupSize - 1) / ThreadGroupSize;
		cmd->Dispatch(groupCountX, groupCountY, 1);
	}
}
