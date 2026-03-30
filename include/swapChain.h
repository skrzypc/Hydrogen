#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

#include <array>

#include "config.h"
#include "verifier.h"
#include "texture.h"

namespace Hydrogen
{
	class SwapChain
	{
	public:
		SwapChain() = default;
		~SwapChain() = default;
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;
		SwapChain(SwapChain&&) noexcept = default;
		SwapChain& operator=(SwapChain&&) noexcept = default;

		void Create(GpuDevice& device, HWND hWnd);
		void Present();
		
		Texture* GetCurrentBackBuffer() const { return m_backBuffers[m_frameIndex]; }

		const uint64 GetCurrentFrameNumber() const { return m_frameNumber; }
		const uint32 GetCurrentFrameIndex() const { return static_cast<uint32>(m_frameIndex); }

	private:
		uint8 m_frameIndex = 0u;
		uint64 m_frameNumber = 0u;

		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_pSwapChain = nullptr;
		std::array<Texture*, Config::FramesInFlight> m_backBuffers{};
		std::array<TextureView, Config::FramesInFlight> m_backBufferRtvs{};
	};
}