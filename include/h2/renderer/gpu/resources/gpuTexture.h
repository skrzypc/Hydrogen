#pragma once

#include <memory>
#include <string>

#include <h2/core/basicTypes.h>
#include <h2/renderer/gpu/device.h>
#include <h2/renderer/gpu/resources/texture.h>

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
