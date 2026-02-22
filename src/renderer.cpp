
#include "renderer.h"
#include "logger.h"
#include "verifier.h"

#include <d3d12.h>
#include <DirectXColors.h>

namespace Hydrogen
{
	void Renderer::Initialize(HWND hWnd)
	{
		m_gpuDevice.Create();
		m_swapChain.Create(m_gpuDevice, hWnd);
	}

	void Renderer::RenderFrame()
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator = nullptr;
		H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)), "Failed to create command allocator!");

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList = nullptr;
		H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)), "Failed to create command list!");

		const TextureView* backBufferView = m_swapChain.GetCurrentBackBuffer();

		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = backBufferView->pTexture->GetResource();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			pCommandList->ResourceBarrier(1, &barrier);
		}

		const FLOAT* clearColor = nullptr;

		switch (backBufferView->index % 3)
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

		pCommandList->ClearRenderTargetView(m_gpuDevice.GetRenderTargetCpuHandle(*backBufferView), clearColor, 0, nullptr);

		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = backBufferView->pTexture->GetResource();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			pCommandList->ResourceBarrier(1, &barrier);
		}

		pCommandList->Close();
		ID3D12CommandList* ppCommandLists[] = { pCommandList.Get() };
		m_gpuDevice.GetDirectCommandQueue()->ExecuteCommandLists(1, ppCommandLists);

		m_swapChain.Present();
	}
}
