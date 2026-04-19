#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "graphicsContext.h"
#include "renderPasses/overlappingRectsPass.h"

namespace Hydrogen
{
	void OverlappingRectsPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		Shader::Desc vsDesc
		{
			.sourcePath = "overlappingRects.vs",
			.name = "OverlappingRectsVS",
			.entryPoint = "main",
			.type = eShaderType::VS,
		};

		Shader::Desc psDesc
		{
			.sourcePath = "overlappingRects.ps",
			.name = "OverlappingRectsPS",
			.entryPoint = "main",
			.type = eShaderType::PS,
		};

		Shader vs(vsDesc);
		shaderCompiler.Compile(vs);

		Shader ps(psDesc);
		shaderCompiler.Compile(ps);

		const DXGI_FORMAT targetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_RENDER_TARGET_BLEND_DESC blendDesc{};
		blendDesc.BlendEnable = TRUE;
		blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_BLEND_DESC psoBlend = PipelineState::DefaultBlend();
		psoBlend.RenderTarget[0] = blendDesc;

		PipelineState::GraphicsDesc psoDesc
		{
			.pVertexShader = &vs,
			.pPixelShader = &ps,
			.renderTargetFormats = std::span<const DXGI_FORMAT>(&targetFormat, 1),
			.depthFormat = DXGI_FORMAT_UNKNOWN,
			.rasterizerDesc = PipelineState::DefaultRasterizer(),
			.blendDesc = psoBlend,
			.depthStencilDesc = PipelineState::DefaultDepthStencil(),
			.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		};
		psoDesc.depthStencilDesc.DepthEnable = FALSE;

		m_pso.CreateGraphics(device, psoDesc);
	}

	void OverlappingRectsPass::Setup(FGBuilder& builder)
	{
		m_targetHandle = builder.Write(target, FGAccess::Output::RenderTarget);

		const Texture::Desc& desc = builder.GetTextureDesc(target);
		m_width = desc.width;
		m_height = desc.height;
	}

	void OverlappingRectsPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		ID3D12GraphicsCommandList10* cmd = gfx.CmdList();

		D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
		D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
		cmd->RSSetViewports(1, &viewport);
		cmd->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.GetRTV(m_targetHandle);
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		cmd->SetPipelineState(m_pso.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		gfx.SetPushConstants(PushConstants{ .rectMin = { -0.7f, -0.7f }, .rectMax = { 0.3f, 0.7f }, .color = { 1.0f, 0.2f, 0.2f, 0.3f } });
		cmd->DrawInstanced(4, 1, 0, 0);

		gfx.SetPushConstants(PushConstants{ .rectMin = { -0.3f, -0.7f }, .rectMax = { 0.7f, 0.7f }, .color = { 0.2f, 0.2f, 1.0f, 0.3f } });
		cmd->DrawInstanced(4, 1, 0, 0);
	}
}
