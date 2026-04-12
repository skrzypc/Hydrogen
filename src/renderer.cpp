
#include <d3d12.h>
#include <DirectXColors.h>

#include <pix3.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

#include "frameGraph.h"

namespace Hydrogen
{
	void Renderer::Initialize(HWND hWnd)
	{
		m_gpuDevice.Create();
		m_swapChain.Create(m_gpuDevice, hWnd);

		m_frameGraph.Initialize(m_gpuDevice);

		m_clearPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_testTrianglePass.Initialize(m_gpuDevice, m_shaderCompiler);
	}

	void Renderer::RenderFrame()
	{
		static uint64 fenceValues[Config::FramesInFlight] = { 0ull, 0ull, 0ull };
		static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[Config::FramesInFlight];
		static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> commandLists[Config::FramesInFlight];
		static bool bCommandListsInitialized = false;

		if (!bCommandListsInitialized)
		{
			for (uint32 i = 0; i < Config::FramesInFlight; ++i)
			{
				H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])), "Failed to create command allocator!");
				H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&commandLists[i])), "Failed to create command list!");
				commandLists[i]->SetName(String::Format(L"COMMAND_LIST_{}", i).c_str());
				commandLists[i]->ClearState(nullptr);
				commandLists[i]->Close();
				commandAllocators[i]->SetName(String::Format(L"COMMAND_ALLOCATOR_{}", i).c_str());
				commandAllocators[i]->Reset();
			}
			bCommandListsInitialized = true;
		}

		uint64 currentFrameNumber = m_swapChain.GetCurrentFrameNumber();
		uint32 currentFrameIndex = m_swapChain.GetCurrentFrameIndex();

		H2_INFO(eLogLevel::Minimal, "Current frame: {}", currentFrameNumber);

		auto& commandQueue = m_gpuDevice.GetDirectCommandQueue();
		commandQueue.Wait(fenceValues[currentFrameIndex]);

		commandAllocators[currentFrameIndex]->Reset();
		commandLists[currentFrameIndex]->Reset(commandAllocators[currentFrameIndex].Get(), nullptr);

		auto pCommandList = commandLists[currentFrameIndex].Get();

		// Bind descriptor heaps and root signature once per frame, before any pass executes.
		{
			std::vector<ID3D12DescriptorHeap*> descriptorHeaps(2, nullptr);
			descriptorHeaps[0] = m_gpuDevice.GetDescriptorHeap(eDescriptorHeapType::CBV_SRV_UAV).GetDxHeap();
			descriptorHeaps[1] = m_gpuDevice.GetDescriptorHeap(eDescriptorHeapType::Sampler).GetDxHeap();

			pCommandList->SetDescriptorHeaps(static_cast<uint32>(descriptorHeaps.size()), descriptorHeaps.data());

			pCommandList->SetGraphicsRootSignature(m_gpuDevice.GetRootSignature().Get());
			//pCommandList->SetComputeRootSignature(pRS);
		}

		// Frame graph usage.
		{
			m_frameGraph.BeginFrame(currentFrameNumber);

			// Define all frame resources.
			{
				m_frameGraph.Import(eFrameResource::Backbuffer, m_swapChain.GetCurrentBackBuffer());
			}

			m_frameGraph.AddPass("ClearBackbuffer", m_clearPass);
			m_frameGraph.AddPass("TestTriangle", m_testTrianglePass);

			m_frameGraph.Compile();
			m_frameGraph.Execute(pCommandList);

			m_frameGraph.Reset();
		}

		pCommandList->Close();

		fenceValues[currentFrameIndex] = m_gpuDevice.GetDirectCommandQueue().SubmitCommandList(pCommandList);

		m_swapChain.Present();
	}
}
