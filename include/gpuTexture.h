#pragma once

#include <memory>
#include <string>

#include "basicTypes.h"
#include "device.h"
#include "texture.h"

namespace Hydrogen
{
    enum class GpuTextureState : uint8
    {
        Empty,      // never registered
        Registered, // slot reserved, upload queued
        Ready,      // pixel data uploaded — safe to sample
    };

    struct GpuTexture
    {
        std::string name;

        std::unique_ptr<Texture> texture;
        ShaderResourceViewHandle srv{};

        GpuTextureState state = GpuTextureState::Empty;
    };
} // namespace Hydrogen
