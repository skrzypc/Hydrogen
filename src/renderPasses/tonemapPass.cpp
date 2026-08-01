#include <d3d12.h>

#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "graphicsContext.h"
#include "renderPasses/tonemapPass.h"

namespace Hydrogen
{
	void TonemapPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		m_pDevice = &device;

		Shader::Desc csDesc
		{
			.sourcePath = "tonemap.cs.hlsl",
			.name = "TonemapCS",
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

	void TonemapPass::Setup(FGBuilder& builder)
	{
		builder.Read("SceneColor", FGAccess::Read::ShaderResource);
		builder.Write("Output", FGAccess::Write::UnorderedAccess);

		const Texture::Desc& desc = builder.GetTextureDesc("Output");
		m_width = desc.width;
		m_height = desc.height;
	}

	void TonemapPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		ID3D12GraphicsCommandList10* cmd = graphicsContext.CmdList();

		cmd->SetComputeRootSignature(m_pDevice->GetRootSignature().Get());
		cmd->SetComputeRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer), GraphicsContext::s_frameDataAddr);
		cmd->SetPipelineState(m_pso.Get());

		const PushConstants push
		{
			.sceneColorIndex = fgExecuteContext.GetSRVIndex("SceneColor"),
			.outputIndex = fgExecuteContext.GetUAVIndex("Output"),
		};
		graphicsContext.SetComputePushConstants(push);

		const uint32 groupCountX = (m_width + ThreadGroupSize - 1) / ThreadGroupSize;
		const uint32 groupCountY = (m_height + ThreadGroupSize - 1) / ThreadGroupSize;
		cmd->Dispatch(groupCountX, groupCountY, 1);
	}
}
