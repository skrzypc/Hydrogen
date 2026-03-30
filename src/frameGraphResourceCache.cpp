
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

		//return CreateTexture(desc, normalizedFlags, key, currentFrame);
		return nullptr;
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

		RenderTargetViewHandle rtvHandle = m_pDevice->CreateRenderTargetView(pTexture, desc, allocatedRtvIndex);
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

		DepthStencilViewHandle dsvHandle = m_pDevice->CreateDepthStencilView(pTexture, desc, allocatedDsvIndex);
		textureDsvs[subresourceRangeHash] = dsvHandle;

		return dsvHandle;
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
