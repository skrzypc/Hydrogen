#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <span>
#include <string>
#include <string_view>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;
	class Shader;

	class RaytracingPipelineState
	{
	public:
		struct HitGroup
		{
			std::string name;
			std::string closestHitExport = "";
			std::string anyHitExport = "";
			std::string intersectionExport = "";
			D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		};

		struct Desc
		{
			const Shader* pLibrary = nullptr;
			std::span<const HitGroup> hitGroups;
			uint32 payloadSizeBytes = 0;
			uint32 attributeSizeBytes = 0;
			uint32 maxRecursionDepth = 1;
		};

		RaytracingPipelineState() = default;
		~RaytracingPipelineState() = default;
		RaytracingPipelineState(const RaytracingPipelineState&) = delete;
		RaytracingPipelineState& operator=(const RaytracingPipelineState&) = delete;
		RaytracingPipelineState(RaytracingPipelineState&&) noexcept = default;
		RaytracingPipelineState& operator=(RaytracingPipelineState&&) noexcept = default;

		void Create(GpuDevice& device, const Desc& desc);

		ID3D12StateObject* Get() const { return m_pDxStateObject.Get(); }

		void* GetShaderIdentifier(std::string_view exportName) const;

	private:
		Microsoft::WRL::ComPtr<ID3D12StateObject> m_pDxStateObject = nullptr;
		Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pDxStateObjectProperties = nullptr;
	};
}
