#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <d3d12.h>

#include "device.h"
#include "frameGraphStructs.h"

namespace Hydrogen
{
	class GpuDevice;

	class FGResourceCache
	{
	public:
		void Initialize(GpuDevice& device, uint32 rtvCapacity = 512, uint32 dsvCapacity = 128);

		Texture* AcquireTexture(Texture::Desc textureDesc, uint64 currentFrame);
		void ReleaseTexture(Texture* pTexture, uint64 currentFrame);

		//Buffer* AcquireBuffer(const Buffer::Desc& desc, uint64 currentFrame);
		//void ReleaseBuffer(Buffer* pBuffer, uint64 currentFrame);

		RenderTargetViewHandle GetRTV(const Texture* pTexture, const FGSubresourceRange& range);
		DepthStencilViewHandle GetDSV(const Texture* pTexture, const FGSubresourceRange& range);

		// TODO: bindless
		// uint32 GetSRVIndex(Texture* pTexture, const FGSubresourceRange& range);
		// uint32 GetUAVIndex(Texture* pTexture, const FGSubresourceRange& range);

	private:
		// TODO: there should be generalized hashing function available. Replace with that.
		uint64 HashRange(const FGSubresourceRange& range);

		static void NormalizeFlags(Texture::Desc& textureDesc)
		{
			if (IsDepthFormat(textureDesc.format))
			{
				textureDesc.flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			}
			else
			{
				textureDesc.flags |= (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			}
		}

	private:
		GpuDevice* m_pDevice = nullptr;

		struct FGFreeTextureEntry
		{
			Texture* pTexture = nullptr;
			uint64 lastUsedFrame = 0;
		};

		FreeListIndexAllocator m_rtvAllocator{};
		FreeListIndexAllocator m_dsvAllocator{};

		std::unordered_map<Texture::Desc, std::vector<FGFreeTextureEntry>, TextureDescHash> m_freeTextures;
		std::unordered_set<const Texture*> m_activeTextures;

		// Each Texture can have multiple views with different subresource ranges. Cache them to avoid redundant descriptor creation.
		std::unordered_map<const Texture*, std::unordered_map<uint64, RenderTargetViewHandle>> m_cachedRtvs;
		std::unordered_map<const Texture*, std::unordered_map<uint64, DepthStencilViewHandle>> m_cachedDsvs;
	};
}