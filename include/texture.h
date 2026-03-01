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

        struct Desc
        {
            uint32 width = 0u;
            uint32 height = 0u;
            uint16 mipLevels = 1u;
            uint16 arraySize = 1u;
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        };

		void SetDesc(const Desc& desc) { m_desc = desc; }

        uint32 GetWidth() const { return m_desc.width; }
        uint32 GetHeight() const { return m_desc.height; }
        DXGI_FORMAT GetFormat() const { return m_desc.format; }
        uint32 GetMipLevelsCount() const { return m_desc.mipLevels; }

    private:
		Desc m_desc{};
    };
}