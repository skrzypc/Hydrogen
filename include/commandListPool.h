#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <vector>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;

	class CommandListPool
	{
	public:
		void Initialize(GpuDevice& device, D3D12_COMMAND_LIST_TYPE type);

		// Returns a free closed command list. Caller resets it with the chosen allocator.
		ID3D12GraphicsCommandList10* Acquire();

		// Returns the command list (must already be closed) to the free list.
		void Release(ID3D12GraphicsCommandList10* pList);

	private:
		GpuDevice* m_pDevice = nullptr;
		D3D12_COMMAND_LIST_TYPE m_type{};

		std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10>> m_allLists;
		std::vector<ID3D12GraphicsCommandList10*> m_freeLists;
	};
}
