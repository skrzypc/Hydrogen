#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

#include <vector>

#include "basicTypes.h"

#include "commandQueue.h"
#include "descriptorHeap.h"
#include "indexAllocators.h"
#include "rootSignature.h"

#include "texture.h"
#include "buffer.h"

namespace Hydrogen
{
	enum class eAdapterVendor : uint16
	{
		INTEL = 0x8086,
		NVIDIA = 0x10DE,
		AMD = 0x1002,
		SOFTWARE = 0x1414,
		INVALID = 0x0000
	};

	enum class eDescriptorHeapType : uint8
	{
		CBV_SRV_UAV = 0,
		Sampler = 1,
		RTV = 2,
		DSV = 3,
	};

	struct RenderTargetViewHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dxCpuHandle{};
	};

	struct DepthStencilViewHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dxCpuHandle{};
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

		const RootSignature& GetRootSignature() const { return m_rootSignature; }

		std::unique_ptr<Texture> CreateTexture(std::wstring_view name, const Texture::Desc& desc, ResourceState& initialState, const D3D12_CLEAR_VALUE* pClearValue = nullptr);
		std::unique_ptr<Texture> CreateTexture(std::wstring_view name, ID3D12Resource* pResource, const Texture::Desc& desc, ResourceState& initialState, const D3D12_CLEAR_VALUE* pClearValue = nullptr);

		std::unique_ptr<Buffer> CreateBuffer(std::wstring_view name, const Buffer::Desc& desc, ResourceState& initialState);
		std::unique_ptr<Buffer> CreateBuffer(std::wstring_view name, ID3D12Resource* pResource, const Buffer::Desc& desc, ResourceState& initialState);

		template<typename AllocatorT>
		AllocatorT RequestDescriptorAllocator(uint32 count, eDescriptorHeapType descHeapType)
		{
			DescriptorHeap& descriptorHeap = GetDescriptorHeap(descHeapType);
			
			uint32 startIndex = descriptorHeap.Allocate(count);

			AllocatorT allocator{};
			allocator.Initialize(startIndex, count);
			
			return allocator;
		}

		RenderTargetViewHandle CreateRenderTargetView(const Texture* pTexture, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, uint32 rtvIndex);
		DepthStencilViewHandle CreateDepthStencilView(const Texture* pTexture, D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, uint32 dsvIndex);

		RenderTargetViewHandle GetRenderTargetHandle(uint32 index) const { return RenderTargetViewHandle{ .dxCpuHandle = m_rtvDescriptorHeap.GetCpuHandle(index) }; }
		DepthStencilViewHandle GetDepthStencilHandle(uint32 index) const { return DepthStencilViewHandle{ .dxCpuHandle = m_dsvDescriptorHeap.GetCpuHandle(index) }; }

		// Temporary? It would be better not to expose heaps directly
		DescriptorHeap& GetDescriptorHeap(eDescriptorHeapType descHeapType);

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

		CommandQueue m_directCommandQueue{};

		// Move it somewhere else? RS should be per render backend I think
		RootSignature m_rootSignature{};
	};
}