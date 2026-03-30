#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;

    class DescriptorHeap
    {
    public:
		DescriptorHeap() = default;
		~DescriptorHeap() = default;
		DescriptorHeap(const DescriptorHeap&) = delete;
		DescriptorHeap& operator=(const DescriptorHeap&) = delete;
		DescriptorHeap(DescriptorHeap&&) noexcept = default;
		DescriptorHeap& operator=(DescriptorHeap&&) noexcept = default;

        void Initialize(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 capacity, std::wstring_view name);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32 index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32 index) const;

		uint32 GetCapacity() const { return m_capacity; }
        uint32 GetDescriptorSize() const { return m_descriptorSize; }

		uint32 Allocate(uint32 count);

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDxDescriptorHeap = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart{};

        uint32 m_descriptorSize = 0u;
        uint32 m_capacity = 0u;

		uint32 m_allocatedSpace = 0u;
    };
}