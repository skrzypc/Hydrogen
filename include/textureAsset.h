#pragma once

#include <limits>
#include <string>
#include <vector>

#include "basicTypes.h"
#include "texture.h"

namespace Hydrogen
{
    struct TextureHandle
    {
        uint32 id = std::numeric_limits<uint32>::max();
        bool IsValid() const
        {
            return id != std::numeric_limits<uint32>::max();
        }
        bool operator==(const TextureHandle&) const = default;
    };

    struct TextureMetadata
    {
        std::string name{};
        std::string path{};
    };

    struct SubresourceLayout
    {
        uint64 offset = 0;
        uint64 size = 0;
    };

    struct TextureData
    {
        Texture::Desc desc{};
        std::vector<std::byte> pixelData{};
        std::vector<SubresourceLayout> subresources{};
    };
} // namespace Hydrogen
