#include "raytracingPipelineState.h"
#include "device.h"
#include "shader.h"
#include "verifier.h"
#include "stringUtilities.h"

#include <string>
#include <vector>

namespace Hydrogen
{
	void RaytracingPipelineState::Create(GpuDevice& device, const Desc& desc)
	{
		H2_VERIFY_FATAL(desc.pLibrary && desc.pLibrary->IsCompiled(), "Raytracing library is not compiled!");
		H2_VERIFY_FATAL(desc.pLibrary->GetDesc().type == eShaderType::RT, "Expected a raytracing library shader!");

		H2_VERIFY_FATAL(!desc.hitGroups.empty(), "Raytracing pipeline needs at least one hit group!");

		// Just one common library for now.
		D3D12_DXIL_LIBRARY_DESC libraryDesc
		{
			.DXILLibrary = { desc.pLibrary->GetBytecode(), desc.pLibrary->GetBytecodeSize() },
			.NumExports = 0,
			.pExports = nullptr,
		};

		struct HitGroupNames
		{
			std::wstring group;
			std::wstring closestHit;
			std::wstring anyHit;
			std::wstring intersection;
		};
		std::vector<HitGroupNames> hitGroupNames;
		hitGroupNames.reserve(desc.hitGroups.size());
		std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs;
		hitGroupDescs.reserve(desc.hitGroups.size());

		for (const HitGroup& hitGroup : desc.hitGroups)
		{
			const HitGroupNames& names = hitGroupNames.emplace_back(HitGroupNames
			{
				.group = String::ToWide(hitGroup.name),
				.closestHit = String::ToWide(hitGroup.closestHitExport),
				.anyHit = String::ToWide(hitGroup.anyHitExport),
				.intersection = String::ToWide(hitGroup.intersectionExport),
			});

			hitGroupDescs.push_back(D3D12_HIT_GROUP_DESC
			{
				.HitGroupExport = names.group.c_str(),
				.Type = hitGroup.type,
				.AnyHitShaderImport = names.anyHit.empty() ? nullptr : names.anyHit.c_str(),
				.ClosestHitShaderImport = names.closestHit.empty() ? nullptr : names.closestHit.c_str(),
				.IntersectionShaderImport = names.intersection.empty() ? nullptr : names.intersection.c_str(),
			});
		}

		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig
		{
			.MaxPayloadSizeInBytes = desc.payloadSizeBytes,
			.MaxAttributeSizeInBytes = desc.attributeSizeBytes,
		};

		D3D12_GLOBAL_ROOT_SIGNATURE globalRootSignature
		{
			.pGlobalRootSignature = device.GetRootSignature().Get(),
		};

		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig
		{
			.MaxTraceRecursionDepth = desc.maxRecursionDepth,
		};

		// One DXIL library + one subobject per hit group + shader config, global
		// root signature, and pipeline config.
		static constexpr uint32 FixedSubobjectCount = 4;
		std::vector<D3D12_STATE_SUBOBJECT> subobjects;
		subobjects.reserve(hitGroupDescs.size() + FixedSubobjectCount);

		subobjects.push_back({ .Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, .pDesc = &libraryDesc });
		for (const D3D12_HIT_GROUP_DESC& hitGroupDesc : hitGroupDescs)
		{
			subobjects.push_back({ .Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, .pDesc = &hitGroupDesc });
		}
		subobjects.push_back({ .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, .pDesc = &shaderConfig });
		subobjects.push_back({ .Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, .pDesc = &globalRootSignature });
		subobjects.push_back({ .Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, .pDesc = &pipelineConfig });

		const D3D12_STATE_OBJECT_DESC stateObjectDesc
		{
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = static_cast<uint32>(subobjects.size()),
			.pSubobjects = subobjects.data(),
		};

		H2_VERIFY_FATAL(device.GetDxDevice()->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&m_pDxStateObject)), "Raytracing state object creation failed!");
		H2_VERIFY_FATAL(m_pDxStateObject.As(&m_pDxStateObjectProperties), "Failed to query ID3D12StateObjectProperties!");
	}

	void* RaytracingPipelineState::GetShaderIdentifier(std::string_view exportName) const
	{
		return m_pDxStateObjectProperties->GetShaderIdentifier(String::ToWide(std::string(exportName)).c_str());
	}
}
