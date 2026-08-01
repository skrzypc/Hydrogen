#include <array>

#include <d3d12.h>

#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "graphicsContext.h"
#include "gpuScene.h"
#include "renderPasses/gBufferPass.h"

namespace Hydrogen
{
	void GBufferPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		Shader::Desc vsDesc
		{
			.sourcePath = "gBuffer.vs.hlsl",
			.name = "GBufferVS",
			.entryPoint = "mainVS",
			.type = eShaderType::VS,
		};

		Shader::Desc psDesc
		{
			.sourcePath = "gBuffer.ps.hlsl",
			.name = "GBufferPS",
			.entryPoint = "mainPS",
			.type = eShaderType::PS,
		};

		Shader vs(vsDesc);
		shaderCompiler.Compile(vs);

		Shader ps(psDesc);
		shaderCompiler.Compile(ps);

		const std::array<DXGI_FORMAT, 3> targetFormats
		{
			AlbedoFormat,
			NormalFormat,
			RoughnessMetalnessFormat,
		};

		D3D12_RASTERIZER_DESC2 rasterizerDesc = PipelineState::DefaultRasterizer();
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		D3D12_DEPTH_STENCIL_DESC1 depthStencilDesc = PipelineState::DefaultDepthStencil();
		depthStencilDesc.DepthEnable = TRUE;

		PipelineState::GraphicsDesc psoDesc
		{
			.pVertexShader = &vs,
			.pPixelShader = &ps,
			.renderTargetFormats = targetFormats,
			.depthFormat = DepthFormat,
			.rasterizerDesc = rasterizerDesc,
			.blendDesc = PipelineState::DefaultBlend(),
			.depthStencilDesc = depthStencilDesc,
			.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		};

		m_pso.CreateGraphics(device, psoDesc);
	}

	void GBufferPass::Setup(FGBuilder& builder)
	{
		builder.Write("GBuffer_Albedo", FGAccess::Write::RenderTarget, FGLoadOp::Clear);
		builder.Write("GBuffer_Normal", FGAccess::Write::RenderTarget, FGLoadOp::Clear);
		builder.Write("GBuffer_RM", FGAccess::Write::RenderTarget, FGLoadOp::Clear);
		builder.Write("SceneDepth", FGAccess::Write::DepthStencil, FGLoadOp::Clear);

		const Texture::Desc& desc = builder.GetTextureDesc("GBuffer_Albedo");
		m_width = desc.width;
		m_height = desc.height;
	}

	void GBufferPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		ID3D12GraphicsCommandList10* cmd = graphicsContext.CmdList();

		D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float32>(m_width), static_cast<float32>(m_height), 0.0f, 1.0f };
		D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
		cmd->RSSetViewports(1, &viewport);
		cmd->RSSetScissorRects(1, &scissor);

		const std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 3> rtvs
		{
			fgExecuteContext.GetRTV("GBuffer_Albedo"),
			fgExecuteContext.GetRTV("GBuffer_Normal"),
			fgExecuteContext.GetRTV("GBuffer_RM"),
		};

		D3D12_CPU_DESCRIPTOR_HANDLE dsv = fgExecuteContext.GetDSV("SceneDepth");

		cmd->OMSetRenderTargets(static_cast<uint32>(rtvs.size()), rtvs.data(), FALSE, &dsv);
		cmd->SetPipelineState(m_pso.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_INDEX_BUFFER_VIEW ibv{};
		ibv.BufferLocation = pScene->GetIndexBuffer()->GetResource()->GetGPUVirtualAddress();
		ibv.SizeInBytes = static_cast<UINT>(pScene->GetIndexBuffer()->GetDesc().size);
		ibv.Format = DXGI_FORMAT_R32_UINT;
		cmd->IASetIndexBuffer(&ibv);

		for (uint32 i = 0; i < static_cast<uint32>(renderObjects.size()); ++i)
		{
			const GpuMesh* gpuMesh = pScene->GetGpuMesh(renderObjects[i].mesh);
			if (!gpuMesh || gpuMesh->state < GpuMeshState::GeometryReady)
			{
				continue;
			}

			PushConstants push{};
			push.transformIndex = i;
			push.baseVertex = gpuMesh->baseVertex; // TODO: Do we need this?
			graphicsContext.SetPushConstants(push);

			cmd->DrawIndexedInstanced(gpuMesh->indexCount, 1, gpuMesh->baseIndex, 0, 0);
		}
	}
}
