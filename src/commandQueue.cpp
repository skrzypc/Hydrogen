
#include "commandQueue.h"

#include "device.h"
#include "verifier.h"
#include "stringUtilities.h"

namespace Hydrogen
{
	void CommandQueue::Initialize(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
	{
		std::wstring queueTypeString = L"INVALID";
		switch (type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			queueTypeString = L"DIRECT";
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		{
			queueTypeString = L"COMPUTE";
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COPY:
		{
			queueTypeString = L"COPY";
			break;
		}
		default:
			break;
		}

		D3D12_COMMAND_QUEUE_DESC commandQueueDesc
		{
			.Type = type,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0u
		};

		H2_VERIFY_FATAL(pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_pQueue)), "Failed to create command queue!");
		m_pQueue->SetName(String::Format(L"H2_{}_COMMAND_QUEUE", queueTypeString).c_str());

		H2_VERIFY_FATAL(pDevice->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)), "Failed to create fence!");
		m_pFence->SetName(String::Format(L"H2_{}_FENCE", queueTypeString).c_str());

		m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, String::Format(L"H2_{}_FENCE_EVENT", queueTypeString).c_str());
		H2_VERIFY_FATAL(m_fenceEvent, "Failed to create fence event!");

		H2_VERIFY(m_pQueue->GetTimestampFrequency(&m_timestampFrequency), "Failed to get timestamp frequency!");
	}

	uint64 CommandQueue::SubmitCommandList(ID3D12CommandList* pCommandList)
	{
		m_pQueue->ExecuteCommandLists(1, &pCommandList);

		return Signal();
	}

	uint64 CommandQueue::Signal()
	{
		uint64 fenceValue = ++m_fenceValue;

		H2_VERIFY(m_pQueue->Signal(m_pFence.Get(), fenceValue), "Failed to set Signal!");

		return fenceValue;
	}

	void CommandQueue::Wait(uint64 fenceValue)
	{
		uint64 completedFenceValue = GetCompletedFenceValue();

		if (completedFenceValue >= fenceValue)
		{
			return;
		}
			
		H2_VERIFY(m_pFence.Get()->SetEventOnCompletion(fenceValue, m_fenceEvent), "Failed to set Event!");

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	void CommandQueue::WaitOnQueue(const CommandQueue& other, uint64 fenceValue)
	{
		H2_VERIFY(m_pQueue->Wait(other.m_pFence.Get(), fenceValue), "Failed to enqueue GPU wait on queue fence!");
	}

	void CommandQueue::WaitForIdle()
	{
		uint64 fenceValue = Signal();

		Wait(fenceValue);
	}
}
