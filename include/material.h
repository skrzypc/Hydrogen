#pragma once

#include <limits>
#include <optional>
#include <string>

#include <DirectXMath.h>

#include "basicTypes.h"
#include "textureAsset.h"

namespace Hydrogen
{
    struct MaterialHandle
    {
        uint32 id = std::numeric_limits<uint32>::max();
        bool IsValid() const
        {
            return id != std::numeric_limits<uint32>::max();
        }
        bool operator==(const MaterialHandle&) const = default;
    };

    struct Material
    {
        std::string name;

        DirectX::XMFLOAT3 baseColor = {1.0f, 1.0f, 1.0f};
        float32 roughness = 1.0f;
        float32 metallic = 0.0f;
        DirectX::XMFLOAT3 emissive = {0.0f, 0.0f, 0.0f};

        std::optional<std::string> albedoTexturePath;
        std::optional<std::string> normalTexturePath;
        std::optional<std::string> metallicRoughnessTexturePath;

        TextureHandle albedoTexture{};
        TextureHandle normalTexture{};
        TextureHandle metallicRoughnessTexture{};
    };
} // namespace Hydrogen
