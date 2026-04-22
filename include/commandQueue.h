#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;

	class CommandQueue
	{
	public:
		CommandQueue() = default;
		~CommandQueue()
		{
			WaitForIdle();
		}
		CommandQueue(const CommandQueue&) = delete;
		CommandQueue& operator=(const CommandQueue&) = delete;
		CommandQueue(CommandQueue&&) noexcept = default;
		CommandQueue& operator=(CommandQueue&&) noexcept = default;

		void Initialize(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);

		uint64 SubmitCommandList(ID3D12CommandList* pCommandList);
		//void SubmitCommandLists(ID3D12CommandList* const* lists, uint32 count);

		uint64 Signal();

		void Wait(uint64 fenceValue);
		void WaitForIdle();

		// Stalls this queue until another queue's fence reaches fenceValue. Does not block the CPU.
		void WaitOnQueue(const CommandQueue& other, uint64 fenceValue);

		uint64 GetCompletedFenceValue() { return m_pFence->GetCompletedValue(); }

		ID3D12CommandQueue* GetDxCommandQueue() const { return m_pQueue.Get(); }
		ID3D12Fence1* GetDxFence() const { return m_pFence.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pQueue = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Fence1> m_pFence = nullptr;
		HANDLE m_fenceEvent = nullptr;
		uint64 m_fenceValue = 0ull;

		uint64 m_timestampFrequency = 0ull;
	};
}