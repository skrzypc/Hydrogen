
#include "frameGraphResourceCache.h"

namespace Hydrogen
{
	void FGResourceCache::Initialize(GpuDevice& device, uint32 rtvCapacity, uint32 dsvCapacity)
	{
		m_pDevice = &device;
		m_rtvAllocator = m_pDevice->RequestDescriptorAllocator<FreeListIndexAllocator>(rtvCapacity, eDescriptorHeapType::RTV);
		m_dsvAllocator = m_pDevice->RequestDescriptorAllocator<FreeListIndexAllocator>(dsvCapacity, eDescriptorHeapType::DSV);
	}

	// Copy desc here. Avoid it?
	Texture* FGResourceCache::AcquireTexture(Texture::Desc textureDesc, uint64 currentFrame)
	{
		NormalizeFlags(textureDesc);

		auto it = m_freeTextures.find(textureDesc);
		if (it != m_freeTextures.end() && !it->second.empty())
		{
			// Reuse existing resource
			FGFreeTextureEntry entry = std::move(it->second.back());
			it->second.pop_back();

			Texture* pTexture = entry.pTexture;
			m_activeTextures.insert(pTexture);

			return pTexture;
		}

		return CreateTexture(textureDesc);
	}

	void FGResourceCache::ReleaseTexture(Texture* pTexture, uint64 currentFrame)
	{
		H2_VERIFY_FATAL(m_activeTextures.contains(pTexture), "Releasing unknown texture");

		m_activeTextures.erase(pTexture);

		m_freeTextures[pTexture->GetDesc()].push_back(
			{
				.pTexture = pTexture,
				.lastUsedFrame = currentFrame,
			});
	}

	Buffer* FGResourceCache::AcquireBuffer(const Buffer::Desc& desc, uint64 currentFrame)
	{
		auto it = m_freeBuffers.find(desc);
		if (it != m_freeBuffers.end() && !it->second.empty())
		{
			// Reuse existing resource
			Buffer* pBuffer = it->second.back();
			it->second.pop_back();
			m_activeBuffers.insert(pBuffer);

			return pBuffer;
		}

		return CreateBuffer(desc);
	}

	void FGResourceCache::ReleaseBuffer(Buffer* pBuffer, uint64 currentFrame)
	{
		H2_VERIFY_FATAL(m_activeBuffers.contains(pBuffer), "Releasing unknown buffer");

		m_activeBuffers.erase(pBuffer);

		m_freeBuffers[pBuffer->GetDesc()].push_back(pBuffer);
	}

	RenderTargetViewHandle FGResourceCache::GetRTV(const Texture* pTexture, const FGSubresourceRange& range)
	{
		uint64 subresourceRangeHash = HashRange(range);

		auto& textureRtvs = m_cachedRtvs[pTexture];
		auto it = textureRtvs.find(subresourceRangeHash);
		if (it != textureRtvs.end())
		{
			return it->second;
		}

		uint32 allocatedRtvIndex = m_rtvAllocator.Allocate();

		D3D12_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = pTexture->GetDesc().format;

		if (pTexture->GetDesc().arraySize > 1)
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = range.mipOffset;
			desc.Texture2DArray.FirstArraySlice = range.arrayOffset;
			desc.Texture2DArray.ArraySize = (range.arraySlicesCount == FGSubresourceRange::All)
				? pTexture->GetDesc().arraySize - range.arrayOffset
				: range.arraySlicesCount;
		}
		else
		{
			desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = range.mipOffset;
		}

		RenderTargetViewHandle rtvHandle = m_pDevice->CreateRenderTargetViewAtIndex(pTexture, desc, allocatedRtvIndex);
		textureRtvs[subresourceRangeHash] = rtvHandle;

		return rtvHandle;
	}

	DepthStencilViewHandle FGResourceCache::GetDSV(const Texture* pTexture, const FGSubresourceRange& range)
	{
		uint64 subresourceRangeHash = HashRange(range);

		auto& textureDsvs = m_cachedDsvs[pTexture];
		auto  it = textureDsvs.find(subresourceRangeHash);
		if (it != textureDsvs.end())
		{
			return it->second;
		}

		uint32 allocatedDsvIndex = m_dsvAllocator.Allocate();

		D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = pTexture->GetDesc().format;
		desc.Flags = D3D12_DSV_FLAG_NONE;

		if (pTexture->GetDesc().arraySize > 1)
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = range.mipOffset;
			desc.Texture2DArray.FirstArraySlice = range.arrayOffset;
			desc.Texture2DArray.ArraySize = (range.arraySlicesCount == FGSubresourceRange::All)
				? pTexture->GetDesc().arraySize - range.arrayOffset
				: range.arraySlicesCount;
		}
		else
		{
			desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = range.mipOffset;
		}

		DepthStencilViewHandle dsvHandle = m_pDevice->CreateDepthStencilViewAtIndex(pTexture, desc, allocatedDsvIndex);
		textureDsvs[subresourceRangeHash] = dsvHandle;

		return dsvHandle;
	}

	ShaderResourceViewHandle FGResourceCache::GetSRV(const Texture* pTexture, const FGSubresourceRange& range)
	{
		uint64 subresourceRangeHash = HashRange(range);

		auto& textureSrvs = m_cachedSrvs[pTexture];
		if (textureSrvs.contains(subresourceRangeHash))
		{
			return textureSrvs.at(subresourceRangeHash);
		}

		const Texture::Desc& textureDesc = pTexture->GetDesc();

		const uint32 mipLevelsCount = (range.mipLevelsCount == FGSubresourceRange::All)
			? textureDesc.mipLevels - range.mipOffset
			: range.mipLevelsCount;

		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = textureDesc.format;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (textureDesc.arraySize > 1)
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MostDetailedMip = range.mipOffset;
			desc.Texture2DArray.MipLevels = mipLevelsCount;
			desc.Texture2DArray.FirstArraySlice = range.arrayOffset;
			desc.Texture2DArray.ArraySize = (range.arraySlicesCount == FGSubresourceRange::All)
				? textureDesc.arraySize - range.arrayOffset
				: range.arraySlicesCount;
		}
		else
		{
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = range.mipOffset;
			desc.Texture2D.MipLevels = mipLevelsCount;
		}

		ShaderResourceViewHandle srvHandle = m_pDevice->CreateShaderResourceView(pTexture, desc);
		textureSrvs[subresourceRangeHash] = srvHandle;

		return srvHandle;
	}

	UnorderedAccessViewHandle FGResourceCache::GetUAV(const Texture* pTexture, const FGSubresourceRange& range)
	{
		uint64 subresourceRangeHash = HashRange(range);

		auto& textureUavs = m_cachedUavs[pTexture];
		if (textureUavs.contains(subresourceRangeHash))
		{
			return textureUavs.at(subresourceRangeHash);
		}

		const Texture::Desc& textureDesc = pTexture->GetDesc();

		D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = textureDesc.format;

		if (textureDesc.arraySize > 1)
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.MipSlice = range.mipOffset;
			desc.Texture2DArray.FirstArraySlice = range.arrayOffset;
			desc.Texture2DArray.ArraySize = (range.arraySlicesCount == FGSubresourceRange::All)
				? textureDesc.arraySize - range.arrayOffset
				: range.arraySlicesCount;
		}
		else
		{
			desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = range.mipOffset;
		}

		UnorderedAccessViewHandle uavHandle = m_pDevice->CreateUnorderedAccessView(pTexture, desc);
		textureUavs[subresourceRangeHash] = uavHandle;

		return uavHandle;
	}

	ShaderResourceViewHandle FGResourceCache::GetAccelerationStructureSRV(const Buffer* pBuffer)
	{
		if (m_cachedAccelerationStructureSrvs.contains(pBuffer))
		{
			return m_cachedAccelerationStructureSrvs.at(pBuffer);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.RaytracingAccelerationStructure.Location = pBuffer->GetResource()->GetGPUVirtualAddress();

		// D3D12 requires acceleration structure SRVs to be created with a null resource.
		ShaderResourceViewHandle srvHandle = m_pDevice->CreateShaderResourceView(static_cast<const Buffer*>(nullptr), desc);
		m_cachedAccelerationStructureSrvs[pBuffer] = srvHandle;

		return srvHandle;
	}

	Texture* FGResourceCache::CreateTexture(const Texture::Desc& desc)
	{
		D3D12_CLEAR_VALUE clearValue{};
		const D3D12_CLEAR_VALUE* pClearValue = nullptr;

		if (desc.flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			clearValue.Format = desc.format;
			clearValue.Color[0] = desc.optimizedClearColor[0];
			clearValue.Color[1] = desc.optimizedClearColor[1];
			clearValue.Color[2] = desc.optimizedClearColor[2];
			clearValue.Color[3] = desc.optimizedClearColor[3];
			pClearValue = &clearValue;
		}
		else if (desc.flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			clearValue.Format = desc.format;
			clearValue.DepthStencil = { .Depth = desc.optimizedDepthClearValue, .Stencil = 0 };
			pClearValue = &clearValue;
		}

		ResourceState initialState{};
		auto pTexture = m_pDevice->CreateTexture(L"FrameGraphTexture", desc, initialState, pClearValue);

		Texture* pRaw = pTexture.get();
		m_ownedTextures.push_back(std::move(pTexture));
		m_activeTextures.insert(pRaw);

		return pRaw;
	}

	Buffer* FGResourceCache::CreateBuffer(const Buffer::Desc& desc)
	{
		ResourceState initialState{};
		auto pBuffer = m_pDevice->CreateBuffer(L"FrameGraphBuffer", desc, initialState);

		Buffer* pRaw = pBuffer.get();
		m_ownedBuffers.push_back(std::move(pBuffer));
		m_activeBuffers.insert(pRaw);

		return pRaw;
	}

	// TODO: there should be generalized hashing function available. Replace with that.
	uint64 FGResourceCache::HashRange(const FGSubresourceRange& range)
	{
		uint64 seed = 0;
		auto combine = [&](auto v)
			{
				seed ^= std::hash<decltype(v)>{}(v)+0x9e3779b9u + (seed << 6) + (seed >> 2);
			};
		combine(range.mipOffset);
		combine(range.mipLevelsCount);
		combine(range.arrayOffset);
		combine(range.arraySlicesCount);

		return seed;
	}
}
