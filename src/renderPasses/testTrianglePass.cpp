#include <cmath>

#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "frameResources.h"
#include "graphicsContext.h"
#include "renderPasses/testTrianglePass.h"

namespace Hydrogen
{
	void TestTrianglePass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		Shader::Desc vsDesc
		{
			.sourcePath = "testShader.vs",
			.name = "TestVs",
			.entryPoint = "main",
			.type = eShaderType::VS,
		};

		Shader::Desc psDesc
		{
			.sourcePath = "testShader.ps",
			.name = "TestPs",
			.entryPoint = "main",
			.type = eShaderType::PS,
		};

		Shader vs(vsDesc);
		shaderCompiler.Compile(vs);

		Shader ps(psDesc);
		shaderCompiler.Compile(ps);

		const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		PipelineState::GraphicsDesc psoDesc
		{
			.pVertexShader = &vs,
			.pPixelShader = &ps,
			.renderTargetFormats = std::span<const DXGI_FORMAT>(&backBufferFormat, 1),
			.depthFormat = DXGI_FORMAT_UNKNOWN,
			.rasterizerDesc = PipelineState::DefaultRasterizer(),
			.blendDesc = PipelineState::DefaultBlend(),
			.depthStencilDesc = PipelineState::DefaultDepthStencil(),
			.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		};
		psoDesc.depthStencilDesc.DepthEnable = FALSE;

		m_pso.CreateGraphics(device, psoDesc);
	}

	void TestTrianglePass::Setup(FGBuilder& builder)
	{
		m_backBuffer = builder.Write(eFrameResource::Backbuffer, FGAccess::Output::RenderTarget);

		const Texture::Desc& desc = builder.GetTextureDesc(eFrameResource::Backbuffer);
		m_width = desc.width;
		m_height = desc.height;
	}

	void TestTrianglePass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		constexpr float32 kSpeed = 1.0f / 1800.0f;
		constexpr float32 k2Pi3  = 2.0944f; // 2π/3

		m_time += kSpeed;
		m_passData.colorTint =
		{
			0.5f + 0.5f * std::sinf(m_time),
			0.5f + 0.5f * std::sinf(m_time + k2Pi3),
			0.5f + 0.5f * std::sinf(m_time + 2.0f * k2Pi3),
			1.0f,
		};

		gfx.SetPassData(m_passData);

		ID3D12GraphicsCommandList10* cmd = gfx.CmdList();

		D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
		D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
		cmd->RSSetViewports(1, &viewport);
		cmd->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.GetRTV(m_backBuffer);
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		cmd->SetPipelineState(m_pso.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);
	}
}
