#include "textureLoader.h"

#include <DirectXTex.h>

#include "stringUtilities.h"
#include "verifier.h"

namespace Hydrogen
{
    TextureData TextureLoader::Load(std::string_view path)
    {
        const std::wstring widePath = String::ToWide(std::string(path));

        DirectX::ScratchImage scratchImage{};
        DirectX::TexMetadata metadata{};
        HRESULT hr = DirectX::LoadFromDDSFile(widePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
        H2_VERIFY_FATAL(hr, "Failed to load DDS texture: {}", path);

        H2_VERIFY_FATAL(metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D,
                        "Only 2D DDS textures are supported: {}", path);
        H2_VERIFY_FATAL(metadata.arraySize == 1, "Texture arrays are not supported: {}", path);

        TextureData result{};
        result.desc = Texture::Desc{
            .width = static_cast<uint32>(metadata.width),
            .height = static_cast<uint32>(metadata.height),
            .mipLevels = static_cast<uint16>(metadata.mipLevels),
            .arraySize = 1u,
            .format = metadata.format,
            .flags = D3D12_RESOURCE_FLAG_NONE,
            .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        };

        uint64 totalSize = 0;
        for (uint32 mipIndex = 0; mipIndex < metadata.mipLevels; ++mipIndex)
        {
            const DirectX::Image* pImage = scratchImage.GetImage(mipIndex, 0, 0);
            H2_VERIFY_FATAL(pImage != nullptr, "Missing mip {} in DDS texture: {}", mipIndex, path);
            totalSize += pImage->slicePitch;
        }

        result.pixelData.resize(totalSize);
        result.subresources.reserve(metadata.mipLevels);

        uint64 writeOffset = 0;
        for (uint32 mipIndex = 0; mipIndex < metadata.mipLevels; ++mipIndex)
        {
            const DirectX::Image* pImage = scratchImage.GetImage(mipIndex, 0, 0);

            memcpy(result.pixelData.data() + writeOffset, pImage->pixels, pImage->slicePitch);
            result.subresources.push_back(SubresourceLayout{.offset = writeOffset, .size = pImage->slicePitch});

            writeOffset += pImage->slicePitch;
        }

        return result;
    }
} // namespace Hydrogen
