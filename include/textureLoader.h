#pragma once

#include <string_view>

#include "textureAsset.h"

namespace Hydrogen
{
    class TextureLoader
    {
    public:
        static TextureData Load(std::string_view path);
    };
} // namespace Hydrogen
