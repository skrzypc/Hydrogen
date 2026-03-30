
#include "descriptorHeap.h"

#include "device.h"

#include "verifier.h"

namespace Hydrogen
{
	void DescriptorHeap::Initialize(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity, std::wstring_view name)
	{
		bool bShaderVisible = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

		m_capacity = capacity;

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc
		{
			.Type = type,
			.NumDescriptors = m_capacity,
			.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};

		m_descriptorSize = pDevice->GetDescriptorHandleIncrementSize(type);

		H2_VERIFY_FATAL(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_pDxDescriptorHeap)), "Failed to create descriptor heap!");
		m_pDxDescriptorHeap->SetName(name.data());

		m_cpuHandleStart = m_pDxDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_gpuHandleStart = bShaderVisible ?
			m_pDxDescriptorHeap->GetGPUDescriptorHandleForHeapStart() :
			D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = std::numeric_limits<uint64>::max() };
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(uint32 index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpuHandleStart;
		handle.ptr += static_cast<SIZE_T>(index * m_descriptorSize);

		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(uint32 index) const
	{
		if (!H2_VERIFY(m_gpuHandleStart.ptr != std::numeric_limits<uint64>::max(), "Attempted to access a GPU descriptor from a non-shader-visible descriptor heap!"))
		{
			return D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = std::numeric_limits<uint64>::max() };
		}

		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_gpuHandleStart;
		handle.ptr += static_cast<UINT64>(index * m_descriptorSize);

		return handle;
	}

	// Returns the starting index of the allocated range.
	uint32 DescriptorHeap::Allocate(uint32 count)
	{
		H2_VERIFY_FATAL(m_allocatedSpace + count <= m_capacity, "Descriptor heap out of space!");

		uint32 startIndex = m_allocatedSpace;

		m_allocatedSpace += count;

		return startIndex;
	}
}
