#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

#include <vector>

#include "basicTypes.h"

#include "commandQueue.h"
#include "descriptorHeap.h"
#include "linearIndexAllocator.h"

#include "texture.h"
#include "buffer.h"

namespace Hydrogen
{
	struct TextureView
	{
		const Texture* pTexture = nullptr;
		uint32 index = 0u;
	};

	struct BufferView
	{
		const Buffer* pBuffer = nullptr;
		uint32 index = 0u;
	};

	enum class eAdapterVendor : uint16
	{
		INTEL = 0x8086,
		NVIDIA = 0x10DE,
		AMD = 0x3EA,
		SOFTWARE = 0xFFFF,
		INVALID = 0x0000
	};

	class GpuDevice
	{
	public:
		GpuDevice() = default;
		~GpuDevice() = default;
		GpuDevice(const GpuDevice&) = delete;
		GpuDevice& operator=(const GpuDevice&) = delete;
		GpuDevice(GpuDevice&&) noexcept = default;
		GpuDevice& operator=(GpuDevice&&) noexcept = default;

		void Create();

		CommandQueue& GetDirectCommandQueue() { return m_directCommandQueue; }

		IDXGIFactory7* GetDxgiFactory() const { return m_pDxgiFactory.Get(); }
		ID3D12Device14* GetDxDevice() const { return m_pDxDevice.Get(); }

		Texture* CreateTexture(const Texture::Desc& desc);
		Texture* RegisterTexture(ID3D12Resource* pResource, const Texture::Desc& desc, D3D12_RESOURCE_STATES currentState);

		Buffer* CreateBuffer(const Buffer::Desc& desc);
		Buffer* RegisterBuffer(ID3D12Resource* pResource, const Buffer::Desc& desc, D3D12_RESOURCE_STATES currentState);

		TextureView CreateRenderTargetView(const Texture* pTexture);
		TextureView CreateDepthStencilView(const Texture* pTexture);
		TextureView CreateShaderResourceView(const Texture* pTexture);
		TextureView CreateUnorderedAccessView(const Texture* pTexture);

		// TODO: temp?
		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetCpuHandle(const TextureView& rtv) const { return m_rtvDescriptorHeap.GetCpuHandle(rtv.index); }

	private:
		void Initialize();
		bool CheckRequiredFeatureSupport() const;

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory7> m_pDxgiFactory = nullptr;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> m_pDxgiAdapter = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Device14> m_pDxDevice = nullptr;

		LinearIndexAllocator m_cbvSrvUavDescriptorAllocator{};
		DescriptorHeap m_cbvSrvUavDescriptorHeap{};

		LinearIndexAllocator m_samplerDescriptorAllocator{};
		DescriptorHeap m_samplerDescriptorHeap{};

		LinearIndexAllocator m_rtvDescriptorAllocator{};
		DescriptorHeap m_rtvDescriptorHeap{};

		LinearIndexAllocator m_dsvDescriptorAllocator{};
		DescriptorHeap m_dsvDescriptorHeap{};

		std::vector<std::unique_ptr<Texture>> m_registeredTextures{};
		std::vector<std::unique_ptr<Buffer>> m_registeredBuffers{};

		CommandQueue m_directCommandQueue{};
	};
}