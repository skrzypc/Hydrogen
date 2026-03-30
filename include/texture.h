#pragma once

#include "gpuResource.h"
#include "basicTypes.h"

namespace Hydrogen
{
	class Texture : public GpuResource
	{
	public:
		Texture() = default;
		~Texture() = default;
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) noexcept = default;
		Texture& operator=(Texture&&) noexcept = default;

		enum class Usage : uint8
		{
			None = 0,
			RenderTarget = 1 << 0,
			DepthRead = 1 << 1,
			DepthWrite = 1 << 1,
			ShaderResource = 1 << 2,
			UnorderedAccess = 1 << 3,
		};

		struct Desc
		{
			uint32 width = 0u;
			uint32 height = 0u;
			uint16 mipLevels = 1u;
			uint16 arraySize = 1u;
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
			D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

			bool operator==(const Texture::Desc&) const = default;
		};

		const Desc& GetDesc() const { return m_desc; }
		void SetDesc(const Desc& desc) { m_desc = desc; }

		uint32 GetWidth() const { return m_desc.width; }
		uint32 GetHeight() const { return m_desc.height; }
		DXGI_FORMAT GetFormat() const { return m_desc.format; }
		uint32 GetMipLevelsCount() const { return m_desc.mipLevels; }

	private:
		Desc m_desc{};
		Usage m_usage{};
	};

	struct TextureDescHash
	{
		uint64 operator()(const Texture::Desc& k) const
		{
			uint64 seed = 0;
			auto combine = [&](auto v)
				{
					seed ^= std::hash<decltype(v)>{}(v)	+ 0x9e3779b9u + (seed << 6) + (seed >> 2);
				};
			combine(k.width);
			combine(k.height);
			combine(k.mipLevels);
			combine(k.arraySize);
			combine(static_cast<uint32>(k.format));
			combine(static_cast<uint32>(k.flags));
			combine(static_cast<uint32>(k.dimension));

			return seed;
		}
	};

	inline bool IsDepthFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D16_UNORM:
			return true;
		default:
			return false;
		}
	}
}