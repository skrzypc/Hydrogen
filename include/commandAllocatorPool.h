#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <vector>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;

	class CommandAllocatorPool
	{
	public:
		void Initialize(GpuDevice& device, D3D12_COMMAND_LIST_TYPE type);

		// The latest fence value the GPU has finished is used to reclaim in-flight allocators.
		ID3D12CommandAllocator* Acquire(uint64 completedFenceValue);

		void Release(ID3D12CommandAllocator* pAllocator, uint64 fenceValue);

	private:
		GpuDevice* m_pDevice = nullptr;
		D3D12_COMMAND_LIST_TYPE m_type{};

		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allAllocators{};
		std::vector<ID3D12CommandAllocator*> m_freeAllocators{};
		std::vector<std::pair<uint64, ID3D12CommandAllocator*>> m_inFlightAllocators;
	};
}
