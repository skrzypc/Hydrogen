#include "commandListPool.h"

#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	void CommandListPool::Initialize(GpuDevice& device, D3D12_COMMAND_LIST_TYPE type)
	{
		m_pDevice = &device;
		m_type = type;
	}

	ID3D12GraphicsCommandList10* CommandListPool::Acquire()
	{
		if (!m_freeLists.empty())
		{
			ID3D12GraphicsCommandList10* pList = m_freeLists.back();
			m_freeLists.pop_back();
			return pList;
		}

		// No free list available — create a new one. CreateCommandList requires a valid
		// allocator; use a throwaway one just for construction and close immediately.
		// The caller will Reset() with the real allocator before recording.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pTempAllocator;
		H2_VERIFY_FATAL(m_pDevice->GetDxDevice()->CreateCommandAllocator(m_type, IID_PPV_ARGS(&pTempAllocator)), "Failed to create temp allocator!");

		auto& pNew = m_allLists.emplace_back();
		H2_VERIFY_FATAL(m_pDevice->GetDxDevice()->CreateCommandList(0, m_type, pTempAllocator.Get(), nullptr, IID_PPV_ARGS(&pNew)), "Failed to create command list!");
		pNew->Close();

		return pNew.Get();
	}

	void CommandListPool::Release(ID3D12GraphicsCommandList10* pList)
	{
		m_freeLists.push_back(pList);
	}
}
