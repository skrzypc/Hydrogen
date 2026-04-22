#include "commandAllocatorPool.h"

#include "device.h"
#include "config.h"
#include "verifier.h"

namespace Hydrogen
{
	void CommandAllocatorPool::Initialize(GpuDevice& device, D3D12_COMMAND_LIST_TYPE type)
	{
		m_pDevice = &device;
		m_type = type;

		m_allAllocators.resize(Config::FramesInFlight);
		m_freeAllocators.reserve(Config::FramesInFlight);

		for (auto& pAllocator : m_allAllocators)
		{
			H2_VERIFY_FATAL(m_pDevice->GetDxDevice()->CreateCommandAllocator(m_type, IID_PPV_ARGS(&pAllocator)), "Failed to create command allocator!");
			m_freeAllocators.push_back(pAllocator.Get());
		}
	}

	ID3D12CommandAllocator* CommandAllocatorPool::Acquire(uint64 completedFenceValue)
	{
		// Promote any completed in-flight allocators back to the free list.
		for (auto it = m_inFlightAllocators.begin(); it != m_inFlightAllocators.end(); )
		{
			if (it->first <= completedFenceValue)
			{
				m_freeAllocators.push_back(it->second);
				it = m_inFlightAllocators.erase(it);
			}
			else
			{
				++it;
			}
		}

		if (!m_freeAllocators.empty())
		{
			ID3D12CommandAllocator* pAllocator = m_freeAllocators.back();
			m_freeAllocators.pop_back();
			H2_VERIFY_FATAL(pAllocator->Reset(), "Failed to reset command allocator!");
			return pAllocator;
		}

		// All pre-allocated allocators are in-flight — create a new one.
		auto& pNew = m_allAllocators.emplace_back();
		H2_VERIFY_FATAL(m_pDevice->GetDxDevice()->CreateCommandAllocator(m_type, IID_PPV_ARGS(&pNew)), "Failed to create command allocator!");
		return pNew.Get();
	}

	void CommandAllocatorPool::Release(ID3D12CommandAllocator* pAllocator, uint64 fenceValue)
	{
		m_inFlightAllocators.emplace_back(fenceValue, pAllocator);
	}
}
