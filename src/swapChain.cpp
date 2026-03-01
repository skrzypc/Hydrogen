
#include <dxgi1_6.h>
#include <d3d12.h>

#include "config.h"
#include "swapChain.h"
#include "verifier.h"

namespace Hydrogen
{
	void SwapChain::Create(GpuDevice& gpuDevice, HWND hWnd)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc
		{
			.Width = 0,
			.Height = 0,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.Stereo = FALSE,
			.SampleDesc = {1, 0},
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
			.BufferCount = Config::FramesInFlight,
			.Scaling = DXGI_SCALING_NONE,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_IGNORE,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
		};

		Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain = nullptr;
		H2_VERIFY_FATAL(
			gpuDevice.GetDxgiFactory()->CreateSwapChainForHwnd(
				gpuDevice.GetDirectCommandQueue().GetDxCommandQueue(),
				hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&pSwapChain),
			"Failed to create swap chain"
		);

		pSwapChain.As(&m_pSwapChain);
		m_pSwapChain->GetDesc1(&swapChainDesc);

		for (uint32 i = 0; i < m_backBuffers.size(); ++i)
		{
			ID3D12Resource* pBackBuffer = nullptr;
			H2_VERIFY_FATAL(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)), "Failed to obtain back buffer from swap chain!");

			Texture* pBackBufferTexture = gpuDevice.RegisterTexture(pBackBuffer,
				Texture::Desc
				{
					.width = swapChainDesc.Width,
					.height = swapChainDesc.Height,
					.mipLevels = 1,
					.arraySize = 1,
					.format = swapChainDesc.Format,
					.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
					.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D
				},
				D3D12_RESOURCE_STATE_PRESENT
			);

			m_backBuffers[i] = pBackBufferTexture;
			m_backBufferRtvs[i] = gpuDevice.CreateRenderTargetView(pBackBufferTexture);
		}
	}

	void SwapChain::Present()
	{
		H2_VERIFY_FATAL(m_pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), "Failed to present swap chain!");
		//H2_VERIFY_FATAL(m_pSwapChain->Present(1, 0), "Failed to present swap chain!");

		// We should probably update that at the beginning of the new frame, not at the end of the current one?
		m_frameNumber++;
		m_frameIndex = m_frameNumber % Config::FramesInFlight;
	}
}
