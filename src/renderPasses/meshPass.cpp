#include <d3d12.h>

#include "device.h"
#include "shaderCompiler.h"
#include "frameGraphBuilder.h"
#include "graphicsContext.h"
#include "gpuScene.h"
#include "renderPasses/meshPass.h"

namespace Hydrogen
{
	void MeshPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		Shader::Desc vsDesc
		{
			.sourcePath = "mesh.vs",
			.name = "MeshVS",
			.entryPoint = "mainVS",
			.type = eShaderType::VS,
		};

		Shader::Desc psDesc
		{
			.sourcePath = "mesh.ps",
			.name = "MeshPS",
			.entryPoint = "mainPS",
			.type = eShaderType::PS,
		};

		Shader vs(vsDesc);
		shaderCompiler.Compile(vs);

		Shader ps(psDesc);
		shaderCompiler.Compile(ps);

		const DXGI_FORMAT targetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_RASTERIZER_DESC2 rasterizerDesc = PipelineState::DefaultRasterizer();
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

		D3D12_DEPTH_STENCIL_DESC1 depthStencilDesc = PipelineState::DefaultDepthStencil();
		depthStencilDesc.DepthEnable = FALSE;

		PipelineState::GraphicsDesc psoDesc
		{
			.pVertexShader = &vs,
			.pPixelShader = &ps,
			.renderTargetFormats = std::span<const DXGI_FORMAT>(&targetFormat, 1),
			.depthFormat = DXGI_FORMAT_UNKNOWN,
			.rasterizerDesc = rasterizerDesc,
			.blendDesc = PipelineState::DefaultBlend(),
			.depthStencilDesc = depthStencilDesc,
			.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		};

		m_pso.CreateGraphics(device, psoDesc);
	}

	void MeshPass::Setup(FGBuilder& builder)
	{
		m_targetHandle = builder.Write(target, FGAccess::Output::RenderTarget);

		const Texture::Desc& desc = builder.GetTextureDesc(target);
		m_width = desc.width;
		m_height = desc.height;
	}

	void MeshPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		ID3D12GraphicsCommandList10* cmd = gfx.CmdList();

		D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
		D3D12_RECT scissor{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
		cmd->RSSetViewports(1, &viewport);
		cmd->RSSetScissorRects(1, &scissor);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.GetRTV(m_targetHandle);
		cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
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
			if (!gpuMesh)
			{
				continue;
			}

			PushConstants push{};
			push.transformIndex = i;
			push.color[0] = 1.0f;
			push.color[1] = 0.7f;
			push.color[2] = 0.4f;
			gfx.SetPushConstants(push);

			cmd->DrawIndexedInstanced(gpuMesh->indexCount, 1, gpuMesh->baseIndex, gpuMesh->baseVertex, 0);
		}
	}
}
