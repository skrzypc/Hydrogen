#include <array>

#include <d3d12.h>

#include "device.h"
#include "shaderCompiler.h"
#include "graphicsContext.h"
#include "frameGraphBuilder.h"
#include "renderPasses/rayTraceDispatchPass.h"

namespace Hydrogen
{
	void RayTraceDispatchPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		m_pDevice = &device;

		Shader::Desc libraryDesc
		{
			.sourcePath = "rayTracing.rt.hlsl",
			.name = "RayTracing",
			.type = eShaderType::RT,
		};

		Shader library(libraryDesc);
		shaderCompiler.Compile(library);

		const std::array<RaytracingPipelineState::HitGroup, 1> hitGroups
		{
			RaytracingPipelineState::HitGroup {.name = "defaultHitGroup", .closestHitExport = "mainClosestHit", }
		};

		RaytracingPipelineState::Desc psoDesc
		{
			.pLibrary = &library,
			.hitGroups = hitGroups,
			.payloadSizeBytes = sizeof(float32) * 3,
			.attributeSizeBytes = sizeof(float32) * 2,
			.maxRecursionDepth = 1,
		};
		m_raytracingPso.Create(device, psoDesc);

		const std::array<std::string, 1> rayGenExports{ "mainRayGen" };
		const std::array<std::string, 1> missExports{ "mainMiss" };
		const std::array<std::string, 1> hitGroupExports{ "defaultHitGroup" };

		ShaderTable::Desc tableDesc
		{
			.rayGenExports = rayGenExports,
			.missExports = missExports,
			.hitGroupExports = hitGroupExports,
		};
		m_shaderTable.Create(device, m_raytracingPso, tableDesc);
	}

	void RayTraceDispatchPass::Setup(FGBuilder& builder)
	{
		builder.Read("TLAS", FGAccess::Read::AccelerationStructure);
		builder.Write(outputTarget, FGAccess::Write::UnorderedAccess);

		const Texture::Desc& desc = builder.GetTextureDesc(outputTarget);
		m_width = desc.width;
		m_height = desc.height;
	}

	void RayTraceDispatchPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
	{
		ID3D12GraphicsCommandList10* cmd = gfx.CmdList();
		cmd->SetComputeRootSignature(m_pDevice->GetRootSignature().Get());
		cmd->SetComputeRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer), GraphicsContext::s_frameDataAddr);
		cmd->SetPipelineState1(m_raytracingPso.Get());

		const PushConstants push
		{
			.tlasIndex = ctx.GetSRVIndex("TLAS"),
			.outputUavIndex = ctx.GetUAVIndex(outputTarget),
		};
		gfx.SetComputePushConstants(push);

		const D3D12_DISPATCH_RAYS_DESC dispatchDesc = m_shaderTable.GetDispatchRaysDesc(0, m_width, m_height);
		cmd->DispatchRays(&dispatchDesc);
	}
}
