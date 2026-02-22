
#include "device.h"

#include "config.h"
#include "verifier.h"
#include "stringUtilities.h"

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

		H2_VERIFY_FATAL(D3D12CreateDevice(m_pDxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_pDxDevice)), "DX12 device could not be created!");

		H2_VERIFY_FATAL(CheckRequiredFeatureSupport(), "Device does not support all expected features!");

		Initialize();
	}

	Texture* GpuDevice::CreateTexture(const Texture::Desc& desc)
	{
		auto pTexture = std::make_unique<Texture>();
		pTexture->SetDesc(desc);

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = desc.dimension;
		resourceDesc.Width = desc.width;
		resourceDesc.Height = desc.height;
		resourceDesc.DepthOrArraySize = desc.arraySize;
		resourceDesc.MipLevels = desc.mipLevels;
		resourceDesc.Format = desc.format;
		resourceDesc.SampleDesc = { .Count = 1, .Quality = 0 };
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = desc.flags;

		H2_VERIFY(m_pDxDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(pTexture->GetResourceAddress())),
			"Texture creation failed!"
		);

		pTexture->SetState(D3D12_RESOURCE_STATE_COMMON);

		m_registeredTextures.emplace_back(std::move(pTexture));

		return m_registeredTextures.back().get();
	}

	Texture* GpuDevice::RegisterTexture(ID3D12Resource* pResource, const Texture::Desc& desc, D3D12_RESOURCE_STATES currentState)
	{
		auto pTexture = std::make_unique<Texture>();
		pTexture->SetDesc(desc);
		pTexture->SetState(currentState);

		pTexture->AttachResource(pResource);

		m_registeredTextures.emplace_back(std::move(pTexture));

		return m_registeredTextures.back().get();
	}

	Buffer* GpuDevice::CreateBuffer(const Buffer::Desc& desc)
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

		pBuffer->SetState(D3D12_RESOURCE_STATE_COMMON);

		m_registeredBuffers.emplace_back(std::move(pBuffer));

		return m_registeredBuffers.back().get();
	}

	Buffer* GpuDevice::RegisterBuffer(ID3D12Resource* pResource, const Buffer::Desc& desc, D3D12_RESOURCE_STATES currentState)
	{
		auto pBuffer = std::make_unique<Buffer>();
		pBuffer->SetDesc(desc);
		pBuffer->SetState(currentState);

		pBuffer->AttachResource(pResource);

		m_registeredBuffers.emplace_back(std::move(pBuffer));

		return m_registeredBuffers.back().get();
	}

	TextureView GpuDevice::CreateRenderTargetView(const Texture* pTexture)
	{
		TextureView view{};
		view.pTexture = pTexture;
		view.index = m_rtvDescriptorAllocator.Allocate();

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = pTexture->GetFormat();
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		m_pDxDevice->CreateRenderTargetView(pTexture->GetResource(), &rtvDesc, m_rtvDescriptorHeap.GetCpuHandle(view.index));

		return view;
	}

	void GpuDevice::Initialize()
	{
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc
		{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0u
		};

		H2_VERIFY_FATAL(m_pDxDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_pDxDirectQueue)), "Failed to create DX12 Command Queue!");
		m_pDxDirectQueue->SetName(L"H2_DIRECT_COMMAND_QUEUE");

		m_cbvSrvUavDescriptorHeap.Initialize(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384, L"H2_CBV_SRV_UAV_DESCRIPTOR_HEAP");
		m_cbvSrvUavDescriptorAllocator.Initialize(0, m_cbvSrvUavDescriptorHeap.GetCapacity());

		m_samplerDescriptorHeap.Initialize(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048, L"H2_SAMPLER_DESCRIPTOR_HEAP");
		m_samplerDescriptorAllocator.Initialize(0, m_samplerDescriptorHeap.GetCapacity());

		m_rtvDescriptorHeap.Initialize(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024, L"H2_RTV_DESCRIPTOR_HEAP");
		m_rtvDescriptorAllocator.Initialize(0, m_rtvDescriptorHeap.GetCapacity());

		m_dsvDescriptorHeap.Initialize(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024, L"H2_DSV_DESCRIPTOR_HEAP");
		m_dsvDescriptorAllocator.Initialize(0, m_dsvDescriptorHeap.GetCapacity());
	}

	bool GpuDevice::CheckRequiredFeatureSupport() const
	{
		bool bAllFeaturesSupported = true;

		// Shader model
		{
			D3D_SHADER_MODEL expectedShaderModel = D3D_SHADER_MODEL_6_8;

			D3D12_FEATURE_DATA_SHADER_MODEL featureData{ expectedShaderModel };
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &featureData, sizeof(featureData)), "Could not check shader model support!");
			
			bAllFeaturesSupported &= featureData.HighestShaderModel >= expectedShaderModel;
		}

		// Raytracing
		{
			D3D12_RAYTRACING_TIER expectedRaytracingTier = D3D12_RAYTRACING_TIER_1_1;

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData, sizeof(featureData)), "Could not check raytracing support!");
			
			bAllFeaturesSupported &= featureData.RaytracingTier >= expectedRaytracingTier;
		}

		// Mesh shaders
		{
			D3D12_MESH_SHADER_TIER expectedMeshShaderTier = D3D12_MESH_SHADER_TIER_1;

			D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData)), "Could not check mesh shader support!");
			
			bAllFeaturesSupported &= featureData.MeshShaderTier >= expectedMeshShaderTier;
			
		}

		// Bindless resources
		{
			D3D12_RESOURCE_BINDING_TIER expectedResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_3;

			D3D12_FEATURE_DATA_D3D12_OPTIONS featureData{};
			H2_VERIFY(m_pDxDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData)), "Could not check bindless resource support!");
			
			bAllFeaturesSupported &= featureData.ResourceBindingTier >= expectedResourceBindingTier;
			
		}

		return bAllFeaturesSupported;
	}
}
