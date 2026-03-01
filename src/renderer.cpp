
#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

#include <d3d12.h>
#include <DirectXColors.h>

#include <pix3.h>

namespace Hydrogen
{
	void Renderer::Initialize(HWND hWnd)
	{
		m_gpuDevice.Create();
		m_swapChain.Create(m_gpuDevice, hWnd);
	}

	void Renderer::RenderFrame()
	{
		static bool bFirst = true;
		static uint64 fenceValues[Config::FramesInFlight] = { 0ull, 0ull, 0ull };
		static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[Config::FramesInFlight];
		static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> commandLists[Config::FramesInFlight];

		if (bFirst)
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

			bFirst = false;
		}

		H2_INFO(eLogLevel::Minimal, "Current frame: {}", m_swapChain.GetCurrentFrameNumber());

		uint32 currentFrameIndex = m_swapChain.GetCurrentFrameIndex();

		auto& commandQueue = m_gpuDevice.GetDirectCommandQueue();
		commandQueue.Wait(fenceValues[currentFrameIndex]);

		commandAllocators[currentFrameIndex]->Reset();
		commandLists[currentFrameIndex]->Reset(commandAllocators[currentFrameIndex].Get(), nullptr);

		auto pCommandList = commandLists[currentFrameIndex].Get();

		{
			PIXScopedEvent(m_gpuDevice.GetDirectCommandQueue().GetDxCommandQueue(), PIX_COLOR_INDEX(0), String::Format("Frame {}", m_swapChain.GetCurrentFrameNumber()).c_str());
			PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(0), String::Format("Frame {}", m_swapChain.GetCurrentFrameNumber()).c_str());

			const TextureView* backBufferView = m_swapChain.GetCurrentBackBuffer();

			{
				PIXScopedEvent(m_gpuDevice.GetDirectCommandQueue().GetDxCommandQueue(), PIX_COLOR_INDEX(1), "Texture barrier 1");
				PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(1), "Texture barrier 1");

				D3D12_TEXTURE_BARRIER textureBarrier{};
				textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_NONE;
				textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;

				textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
				textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET;

				textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_PRESENT;
				textureBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

				textureBarrier.pResource = backBufferView->pTexture->GetResource();
				textureBarrier.Subresources.IndexOrFirstMipLevel = 0;
				textureBarrier.Subresources.NumMipLevels = 1;
				textureBarrier.Subresources.FirstArraySlice = 0;
				textureBarrier.Subresources.NumArraySlices = 1;
				textureBarrier.Subresources.FirstPlane = 0;
				textureBarrier.Subresources.NumPlanes = 1;

				D3D12_BARRIER_GROUP barrierGroup = {};
				barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
				barrierGroup.NumBarriers = 1;
				barrierGroup.pTextureBarriers = &textureBarrier;

				pCommandList->Barrier(1, &barrierGroup);
			}

			const FLOAT* clearColor = nullptr;

			switch (currentFrameIndex)
			{
			case 0:
				clearColor = DirectX::Colors::CornflowerBlue;
				break;
			case 1:
				clearColor = DirectX::Colors::DarkRed;
				break;
			case 2:
				clearColor = DirectX::Colors::DarkGreen;
				break;
			default:
				clearColor = DirectX::Colors::White;
				break;
			}

			{
				PIXScopedEvent(m_gpuDevice.GetDirectCommandQueue().GetDxCommandQueue(), PIX_COLOR_INDEX(3), "Clear target");
				PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(3), "Clear target");

				pCommandList->ClearRenderTargetView(m_gpuDevice.GetRenderTargetCpuHandle(*backBufferView), clearColor, 0, nullptr);
			}

			{
				PIXScopedEvent(m_gpuDevice.GetDirectCommandQueue().GetDxCommandQueue(), PIX_COLOR_INDEX(2), "Texture barrier 2");
				PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(2), "Texture barrier 2");

				D3D12_TEXTURE_BARRIER textureBarrier{};
				textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
				textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_NONE;

				textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
				textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;

				textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
				textureBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;

				textureBarrier.pResource = backBufferView->pTexture->GetResource();
				textureBarrier.Subresources.IndexOrFirstMipLevel = 0;
				textureBarrier.Subresources.NumMipLevels = 1;
				textureBarrier.Subresources.FirstArraySlice = 0;
				textureBarrier.Subresources.NumArraySlices = 1;
				textureBarrier.Subresources.FirstPlane = 0;
				textureBarrier.Subresources.NumPlanes = 1;

				D3D12_BARRIER_GROUP barrierGroup = {};
				barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
				barrierGroup.NumBarriers = 1;
				barrierGroup.pTextureBarriers = &textureBarrier;

				pCommandList->Barrier(1, &barrierGroup);
			}
		}

		pCommandList->Close();

		fenceValues[currentFrameIndex] = m_gpuDevice.GetDirectCommandQueue().SubmitCommandList(pCommandList);

		m_swapChain.Present();
	}
}
