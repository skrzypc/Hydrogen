#pragma once

#include <d3d12.h>
#include <memory>
#include <span>
#include <string>

#include "basicTypes.h"
#include "uploadBuffer.h"

namespace Hydrogen
{
	class GpuDevice;
	class RaytracingPipelineState;

	// Builds and owns a raytracing shader table: a GPU buffer holding the shader
	// identifiers produced by a RaytracingPipelineState, laid out as three
	// regions (ray generation, miss, hit group) that DispatchRays indexes into.
	class ShaderTable
	{
	public:
		struct Desc
		{
			std::span<const std::string> rayGenExports;
			std::span<const std::string> missExports;
			std::span<const std::string> hitGroupExports;
		};

		ShaderTable() = default;
		~ShaderTable() = default;
		ShaderTable(const ShaderTable&) = delete;
		ShaderTable& operator=(const ShaderTable&) = delete;
		ShaderTable(ShaderTable&&) noexcept = default;
		ShaderTable& operator=(ShaderTable&&) noexcept = default;

		void Create(GpuDevice& device, const RaytracingPipelineState& pso, const Desc& desc);

		// Selects which ray generation record to launch; the miss and hit group
		// regions are always exposed in full so shaders can index them at runtime.
		[[nodiscard]] D3D12_DISPATCH_RAYS_DESC GetDispatchRaysDesc(uint32 rayGenIndex, uint32 width, uint32 height) const;

	private:
		std::unique_ptr<UploadBuffer> m_buffer;

		uint32 m_recordStride = 0;

		uint64 m_rayGenTableOffset = 0;
		uint32 m_rayGenCount = 0;

		uint64 m_missTableOffset = 0;
		uint32 m_missCount = 0;

		uint64 m_hitGroupTableOffset = 0;
		uint32 m_hitGroupCount = 0;
	};
}
