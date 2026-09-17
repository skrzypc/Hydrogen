#pragma once

#include <optional>

#include <DirectXMath.h>

#include "basicTypes.h"

namespace Hydrogen
{
    // CIE photopic peak (555nm), used to convert Light::intensity to radiometric
    // units for the GPU. See GpuScene::UpdateLights.
    static constexpr float32 kLuminousEfficacy = 683.0f; // lm/W

    enum class eLightType : uint8
    {
        Directional,
        Point,
        Spot,
    };

    struct Light
    {
        eLightType type = eLightType::Point;

        DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};

        // Point and spot lights: Candela (lm/sr).
        // Directional lights: lux (lm/m^2).
        float32 intensity = 0.0f;

        std::optional<float32> range{};
        std::optional<float32> innerConeAngle{};
        std::optional<float32> outerConeAngle{};
    };
} // namespace Hydrogen
