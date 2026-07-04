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

		Texture* AcquireTexture(const Texture::Desc textureDesc, uint64 currentFrame);
		void ReleaseTexture(Texture* pTexture, uint64 currentFrame);

		Buffer* AcquireBuffer(const Buffer::Desc& desc, uint64 currentFrame);
		void ReleaseBuffer(Buffer* pBuffer, uint64 currentFrame);

		RenderTargetViewHandle GetRTV(const Texture* pTexture, const FGSubresourceRange& range);
		DepthStencilViewHandle GetDSV(const Texture* pTexture, const FGSubresourceRange& range);

		// TODO: bindless
		// uint32 GetSRVIndex(Texture* pTexture, const FGSubresourceRange& range);
		// uint32 GetUAVIndex(Texture* pTexture, const FGSubresourceRange& range);

	private:
		Texture* CreateTexture(const Texture::Desc& desc);
		Buffer* CreateBuffer(const Buffer::Desc& desc);

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

		std::vector<std::unique_ptr<Texture>> m_ownedTextures{};
		std::unordered_map<Texture::Desc, std::vector<FGFreeTextureEntry>, TextureDescHash> m_freeTextures{};
		std::unordered_set<const Texture*> m_activeTextures{};

		struct BufferDescHash
		{
			uint64 operator()(const Buffer::Desc& desc) const noexcept
			{
				uint64 seed = std::hash<uint64>{}(desc.size);
				seed ^= std::hash<uint32>{}(desc.flags) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
				seed ^= std::hash<uint32>{}(desc.heapType) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
				return seed;
			}

			bool operator()(const Buffer::Desc& a, const Buffer::Desc& b) const noexcept
			{
				return a.size == b.size && a.flags == b.flags && a.heapType == b.heapType;
			}
		};

		std::vector<std::unique_ptr<Buffer>> m_ownedBuffers{};
		std::unordered_map<Buffer::Desc, std::vector<Buffer*>, BufferDescHash, BufferDescHash> m_freeBuffers{};
		std::unordered_set<const Buffer*> m_activeBuffers{};

		// Each Texture can have multiple views with different subresource ranges. Cache them to avoid redundant descriptor creation.
		std::unordered_map<const Texture*, std::unordered_map<uint64, RenderTargetViewHandle>> m_cachedRtvs;
		std::unordered_map<const Texture*, std::unordered_map<uint64, DepthStencilViewHandle>> m_cachedDsvs;
	};
}