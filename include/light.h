#pragma once

#include <optional>

#include <DirectXMath.h>

#include "basicTypes.h"

namespace Hydrogen
{
    enum class eLightType : uint8
    {
        Directional,
        Point,
        Spot,
    };

    struct Light
    {
        eLightType type = eLightType::Point;

        DirectX::XMFLOAT3 color = { 1.0f, 1.0f, 1.0f };

        // Candela (lm/sr) for point and spot, lux (lm/m^2) for directional. Kept in the authored
        // units so the type stays meaningful; exposure handles the absolute scale at shading time.
        float32 intensity = 0.0f;

        std::optional<float32> range{};
        std::optional<float32> innerConeAngle{};
        std::optional<float32> outerConeAngle{};
    };
}
