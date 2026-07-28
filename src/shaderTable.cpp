#include "shaderTable.h"
#include "device.h"
#include "raytracingPipelineState.h"
#include "verifier.h"

#include <d3d12.h>

namespace Hydrogen
{
	namespace
	{
		constexpr uint64 AlignUp(uint64 value, uint64 alignment)
		{
			return (value + alignment - 1) & ~(alignment - 1);
		}

		constexpr uint32 ShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		// A record is the 32-byte shader identifier plus optional local root
		// arguments. We carry none — every resource is bound bindlessly through
		// the global root signature — so a record is just the identifier. Each
		// record start (including every ray generation record, which DispatchRays
		// addresses directly by StartAddress) must sit on a 64-byte boundary
		// (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT), so the stride is padded
		// up to 64 rather than the 32-byte minimum record alignment.
		constexpr uint32 RecordStride = static_cast<uint32>(
			AlignUp(ShaderIdentifierSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT));
	}

	void ShaderTable::Create(GpuDevice& device, const RaytracingPipelineState& pso, const Desc& desc)
	{
		m_recordStride = RecordStride;

		m_rayGenCount = static_cast<uint32>(desc.rayGenExports.size());
		m_missCount = static_cast<uint32>(desc.missExports.size());
		m_hitGroupCount = static_cast<uint32>(desc.hitGroupExports.size());

		H2_VERIFY_FATAL(m_rayGenCount > 0, "Shader table needs at least one ray generation export!");

		// Each region must begin on a 64-byte boundary; the buffer base itself is
		// already at least 64-byte aligned, so only the miss and hit group starts
		// need padding after the preceding region.
		m_rayGenTableOffset = 0;
		const uint64 rayGenTableSize = static_cast<uint64>(m_rayGenCount) * RecordStride;

		m_missTableOffset = AlignUp(m_rayGenTableOffset + rayGenTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		const uint64 missTableSize = static_cast<uint64>(m_missCount) * RecordStride;

		m_hitGroupTableOffset = AlignUp(m_missTableOffset + missTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		const uint64 hitGroupTableSize = static_cast<uint64>(m_hitGroupCount) * RecordStride;

		const uint64 totalSize = m_hitGroupTableOffset + hitGroupTableSize;

		m_buffer = device.CreateUploadBuffer(L"ShaderTable", totalSize);

		const auto writeRegion = [this, &pso](std::span<const std::string> exports, uint64 tableOffset)
		{
			for (uint32 recordIndex = 0; recordIndex < exports.size(); ++recordIndex)
			{
				const uint64 recordOffset = tableOffset + static_cast<uint64>(recordIndex) * RecordStride;
				m_buffer->Write(pso.GetShaderIdentifier(exports[recordIndex]), ShaderIdentifierSize, recordOffset);
			}
		};

		writeRegion(desc.rayGenExports, m_rayGenTableOffset);
		writeRegion(desc.missExports, m_missTableOffset);
		writeRegion(desc.hitGroupExports, m_hitGroupTableOffset);
	}

	D3D12_DISPATCH_RAYS_DESC ShaderTable::GetDispatchRaysDesc(uint32 rayGenIndex, uint32 width, uint32 height) const
	{
		H2_VERIFY_FATAL(rayGenIndex < m_rayGenCount, "Ray generation index out of range!");

		const D3D12_GPU_VIRTUAL_ADDRESS base = m_buffer->GetGpuAddress();

		D3D12_DISPATCH_RAYS_DESC dispatchDesc{};

		dispatchDesc.RayGenerationShaderRecord.StartAddress = base + m_rayGenTableOffset + static_cast<uint64>(rayGenIndex) * m_recordStride;
		dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_recordStride;

		dispatchDesc.MissShaderTable.StartAddress = base + m_missTableOffset;
		dispatchDesc.MissShaderTable.SizeInBytes = static_cast<uint64>(m_missCount) * m_recordStride;
		dispatchDesc.MissShaderTable.StrideInBytes = m_recordStride;

		dispatchDesc.HitGroupTable.StartAddress = base + m_hitGroupTableOffset;
		dispatchDesc.HitGroupTable.SizeInBytes = static_cast<uint64>(m_hitGroupCount) * m_recordStride;
		dispatchDesc.HitGroupTable.StrideInBytes = m_recordStride;

		dispatchDesc.Width = width;
		dispatchDesc.Height = height;
		dispatchDesc.Depth = 1;

		return dispatchDesc;
	}
}
