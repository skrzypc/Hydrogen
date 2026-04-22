
#include "device.h"

#include "config.h"
#include "verifier.h"
#include "stringUtilities.h"
#include "graphicsContext.h"
#include "copyContext.h"

namespace Hydrogen
{
	void GpuDevice::Create()
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);

		uint32 dxgiFactoryFlags = 0u;
#if _DEBUG
		dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

		Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugInterface = nullptr;
		if (H2_VERIFY_FATAL(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugInterface)), "Unable to obtain DX12 debug interface!"))
		{
			pDebugInterface->EnableDebugLayer();
			pDebugInterface->SetEnableGPUBasedValidation(Config::EnableGPUBasedValidation);
			pDebugInterface->SetEnableAutoName(true);
		}

		Microsoft::WRL::ComPtr<IDXGIInfoQueue> pDXGIInforQueue = nullptr;
		if (H2_VERIFY_FATAL(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDXGIInforQueue)), "Unable to obtain DXGI debug interface!"))
		{
			pDXGIInforQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			pDXGIInforQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

			DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
			{
				80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
			};
			DXGI_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			pDXGIInforQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
		}

		H2_INFO(eLogLevel::Verbose, "DX12 debug layer enabled!");
#else
		dxgiFactoryFlags = 0;
		H2_INFO(eLogLevel::Verbose, "DX12 debug layer not enabled!");
#endif

		H2_VERIFY_FATAL(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_pDxgiFactory)), "Failed to create DXGI factory!");

		std::string adapterVendorString = "INVALID";
		switch (Config::RequestedAdapter)
		{
		case eAdapterVendor::INTEL:
			adapterVendorString = "INTEL";
			break;
		case eAdapterVendor::NVIDIA:
			adapterVendorString = "NVIDIA";
			break;
		case eAdapterVendor::AMD:
			adapterVendorString = "AMD";
			break;
		case eAdapterVendor::SOFTWARE:
			adapterVendorString = "SOFTWARE";
			break;
		}

		H2_INFO(eLogLevel::Verbose, "Looking for {} adapter...", adapterVendorString);

		for (uint32 i = 0; DXGI_ERROR_NOT_FOUND != m_pDxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_pDxgiAdapter)); ++i)
		{
			DXGI_ADAPTER_DESC3 adapterDesc{};
			m_pDxgiAdapter->GetDesc3(&adapterDesc);
			if (static_cast<eAdapterVendor>(adapterDesc.VendorId) == Config::RequestedAdapter)
			{
				std::string gpuName = String::ToUTF8(adapterDesc.Description);
				H2_INFO(eLogLevel::Verbose, "Adapter found:\n> {} ({} MB)", gpuName, (adapterDesc.DedicatedVideoMemory >> 20));

				break;
			}
		}

		H2_VERIFY_FATAL(m_pDxgiAdapter != nullptr, "Requested adapter could not be found! Perhaps requested adapter vendor is not present on this system.");

		H2_VERIFY_FATAL(D3D12CreateDevice(m_pDxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDxDevice)), "DX12 device could not be created!");

		H2_VERIFY_FATAL(CheckRequiredFeatureSupport(), "Device does not support all expected features!");

		Initialize();
	}

	std::unique_ptr<Texture> GpuDevice::CreateTexture(std::wstring_view name, const Texture::Desc& desc, ResourceState& initialState, const D3D12_CLEAR_VALUE* pClearValue)
	{
		auto pTexture = std::make_unique<Texture>();

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC1 resourceDesc{};
		resourceDesc.Dimension = desc.dimension;
		resourceDesc.Width = desc.width;
		resourceDesc.Height = desc.height;
		resourceDesc.DepthOrArraySize = desc.arraySize;
		resourceDesc.MipLevels = desc.mipLevels;
		resourceDesc.Format = desc.format;
		resourceDesc.SampleDesc = { .Count = 1, .Quality = 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = desc.flags;

		H2_VERIFY(m_pDxDevice->CreateCommittedResource3(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState.layout,
			pClearValue,
			nullptr,
			0,
			nullptr,
			IID_PPV_ARGS(pTexture->GetResourceAddress())),
			"Texture creation failed!"
		);

		pTexture->SetName(name);
		pTexture->SetDesc(desc);
		pTexture->SetState(initialState);

		return pTexture;
	}

	std::unique_ptr<Texture> GpuDevice::CreateTexture(std::wstring_view name, ID3D12Resource* pResource, const Texture::Desc& desc, ResourceState& initialState, const D3D12_CLEAR_VALUE* pClearValue)
	{
		auto pTexture = std::make_unique<Texture>();
		pTexture->AttachResource(pResource);

		pTexture->SetName(name);
		pTexture->SetDesc(desc);
		pTexture->SetState(initialState);

		return pTexture;
	}

	std::unique_ptr<Buffer> GpuDevice::CreateBuffer(std::wstring_view name, const Buffer::Desc& desc, ResourceState& initialState)
	{
		auto pBuffer = std::make_unique<Buffer>();
		pBuffer->SetDesc(desc);

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = desc.heapType;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = desc.size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc = { .Count = 1, .Quality = 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = desc.flags;

		H2_VERIFY(m_pDxDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pBuffer->GetResourceAddress())),
			"Buffer creation failed!"
		);

		pBuffer->SetName(name);
		pBuffer->SetState(initialState);

		return pBuffer;
	}

	std::unique_ptr<Buffer> GpuDevice::CreateBuffer(std::wstring_view name, ID3D12Resource* pResource, const Buffer::Desc& desc, ResourceState& initialState)
	{
		auto pBuffer = std::make_unique<Buffer>();
		pBuffer->AttachResource(pResource);

		pBuffer->SetName(name);
		pBuffer->SetDesc(desc);
		pBuffer->SetState(initialState);

		return pBuffer;
	}

	RenderTargetViewHandle GpuDevice::CreateRenderTargetView(const Texture* pTexture, D3D12_RENDER_TARGET_VIEW_DESC rtvDesc, uint32 rtvIndex)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_rtvDescriptorHeap.GetCpuHandle(rtvIndex);
		m_pDxDevice->CreateRenderTargetView(pTexture->GetResource(), &rtvDesc, cpuHandle);

		return RenderTargetViewHandle{ .dxCpuHandle = cpuHandle };
	}

	DepthStencilViewHandle GpuDevice::CreateDepthStencilView(const Texture* pTexture, D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc, uint32 dsvIndex)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_dsvDescriptorHeap.GetCpuHandle(dsvIndex);
		m_pDxDevice->CreateDepthStencilView(pTexture->GetResource(), &dsvDesc, cpuHandle);

		return DepthStencilViewHandle{ .dxCpuHandle = cpuHandle };
	}

	void GpuDevice::Initialize()
	{
		m_directCommandQueue.Initialize(
			GetDxDevice(),
			D3D12_COMMAND_LIST_TYPE_DIRECT
		);

		m_copyCommandQueue.Initialize(
			GetDxDevice(),
			D3D12_COMMAND_LIST_TYPE_COPY
		);

		m_directAllocatorPool.Initialize(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_copyAllocatorPool.Initialize(*this, D3D12_COMMAND_LIST_TYPE_COPY);
		m_directListPool.Initialize(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_copyListPool.Initialize(*this, D3D12_COMMAND_LIST_TYPE_COPY);

		m_cbvSrvUavDescriptorHeap.Initialize(
			GetDxDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			16384,
			L"H2_CBV_SRV_UAV_DESCRIPTOR_HEAP"
		);
		m_cbvSrvUavDescriptorAllocator = RequestDescriptorAllocator<LinearIndexAllocator>(8192, eDescriptorHeapType::CBV_SRV_UAV);

		m_samplerDescriptorHeap.Initialize(
			GetDxDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			2048,
			L"H2_SAMPLER_DESCRIPTOR_HEAP"
		);
		m_samplerDescriptorAllocator = RequestDescriptorAllocator<LinearIndexAllocator>(1024, eDescriptorHeapType::Sampler);

		m_rtvDescriptorHeap.Initialize(
			GetDxDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			1024,
			L"H2_RTV_DESCRIPTOR_HEAP"
		);
		m_rtvDescriptorAllocator = RequestDescriptorAllocator<LinearIndexAllocator>(512, eDescriptorHeapType::RTV);

		m_dsvDescriptorHeap.Initialize(
			GetDxDevice(),
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			1024,
			L"H2_DSV_DESCRIPTOR_HEAP"
		);
		m_dsvDescriptorAllocator = RequestDescriptorAllocator<LinearIndexAllocator>(512, eDescriptorHeapType::DSV);

		m_rootSignature.Create(*this);
	}

	bool GpuDevice::CheckRequiredFeatureSupport() const
	{
		bool bAllFeaturesSupported = true;

		// Shader model
		{
			D3D_SHADER_MODEL expectedShaderModel = static_cast<D3D_SHADER_MODEL>((Config::ShaderModelVersionMajor << 4) | Config::ShaderModelVersionMinor);

			D3D12_FEATURE_DATA_SHADER_MODEL featureData{ expectedShaderModel };
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &featureData, sizeof(featureData)), "Could not check shader model support!");

			bAllFeaturesSupported &= featureData.HighestShaderModel >= expectedShaderModel;
		}

		// Raytracing
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData, sizeof(featureData)), "Could not check raytracing support!");

			bAllFeaturesSupported &= featureData.RaytracingTier >= Config::ExpectedRaytracingTier;
		}

		// Mesh shaders
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData)), "Could not check mesh shader support!");

			bAllFeaturesSupported &= featureData.MeshShaderTier >= Config::ExpectedMeshShaderTier;
		}

		// Bindless resources
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData)), "Could not check bindless resource support!");

			bAllFeaturesSupported &= featureData.ResourceBindingTier >= Config::ExpectedResourceBindingTier;
		}

		// Enhanced barriers
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS12 featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &featureData, sizeof(featureData)), "Could not check enhanced barriers support!");

			bAllFeaturesSupported &= static_cast<bool>(featureData.EnhancedBarriersSupported);
		}

		return bAllFeaturesSupported;
	}

	GraphicsContext GpuDevice::AcquireGraphicsContext()
	{
		uint64 completed = m_directCommandQueue.GetCompletedFenceValue();
		ID3D12CommandAllocator* pAllocator = m_directAllocatorPool.Acquire(completed);
		ID3D12GraphicsCommandList10* pList = m_directListPool.Acquire();
		H2_VERIFY_FATAL(pList->Reset(pAllocator, nullptr), "Failed to reset graphics command list!");
		m_directContextMap[pList] = pAllocator;

		ID3D12DescriptorHeap* descriptorHeaps[] =
		{
			GetDescriptorHeap(eDescriptorHeapType::CBV_SRV_UAV).GetDxHeap(),
			GetDescriptorHeap(eDescriptorHeapType::Sampler).GetDxHeap(),
		};
		pList->SetDescriptorHeaps(2, descriptorHeaps);
		pList->SetGraphicsRootSignature(m_rootSignature.Get());

		return GraphicsContext(pList);
	}

	uint64 GpuDevice::ExecuteGraphicsContext(GraphicsContext ctx)
	{
		ID3D12GraphicsCommandList10* pList = ctx.CmdList();
		H2_VERIFY_FATAL(pList->Close(), "Failed to close graphics command list!");

		uint64 fenceValue = m_directCommandQueue.SubmitCommandList(pList);

		ID3D12CommandAllocator* pAllocator = m_directContextMap.at(pList);
		m_directContextMap.erase(pList);

		m_directListPool.Release(pList);
		m_directAllocatorPool.Release(pAllocator, fenceValue);

		ctx.m_pCommandList.Reset();

		return fenceValue;
	}

	CopyContext GpuDevice::AcquireCopyContext()
	{
		uint64 completed = m_copyCommandQueue.GetCompletedFenceValue();

		ID3D12CommandAllocator* pAllocator = m_copyAllocatorPool.Acquire(completed);
		ID3D12GraphicsCommandList10* pList = m_copyListPool.Acquire();

		H2_VERIFY_FATAL(pList->Reset(pAllocator, nullptr), "Failed to reset copy command list!");

		m_copyContextMap[pList] = pAllocator;

		return CopyContext(pList);
	}

	uint64 GpuDevice::ExecuteCopyContext(CopyContext ctx)
	{
		ID3D12GraphicsCommandList10* pList = ctx.CmdList();
		H2_VERIFY_FATAL(pList->Close(), "Failed to close copy command list!");

		uint64 fenceValue = m_copyCommandQueue.SubmitCommandList(pList);

		ID3D12CommandAllocator* pAllocator = m_copyContextMap.at(pList);
		m_copyContextMap.erase(pList);

		m_copyListPool.Release(pList);
		m_copyAllocatorPool.Release(pAllocator, fenceValue);

		ctx.m_pCommandList.Reset();

		return fenceValue;
	}

	DescriptorHeap& GpuDevice::GetDescriptorHeap(eDescriptorHeapType descHeapType)
	{
		switch (descHeapType)
		{
		case Hydrogen::eDescriptorHeapType::CBV_SRV_UAV:
		{
			return m_cbvSrvUavDescriptorHeap;
		}
		case Hydrogen::eDescriptorHeapType::Sampler:
		{
			return m_samplerDescriptorHeap;
		}
		case Hydrogen::eDescriptorHeapType::RTV:
		{
			return m_rtvDescriptorHeap;
		}
		case Hydrogen::eDescriptorHeapType::DSV:
		{
			return m_dsvDescriptorHeap;
		}
		default:
			std::unreachable();
			break;
		}
	}
}
