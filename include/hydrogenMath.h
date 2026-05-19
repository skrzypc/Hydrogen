#pragma once

#include <SimpleMath.h>

#include "basicTypes.h"

namespace Hydrogen
{
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;
    using Matrix = DirectX::SimpleMath::Matrix;
    using Quaternion = DirectX::SimpleMath::Quaternion;
    using Color = DirectX::SimpleMath::Color;

    inline float32 ToRadians(float32 degrees) noexcept { return DirectX::XMConvertToRadians(degrees); }
    inline float32 ToDegrees(float32 radians) noexcept { return DirectX::XMConvertToDegrees(radians); }

    // LH coordinate system constants (+Z forward, +Y up, +X right).
    // SimpleMath's built-in Forward/Backward follow RH convention, so we define our own.
    // NOTE: These are unqualified names inside namespace Hydrogen — local variables named
    // Forward/Right/Up will silently shadow them without a warning.
    inline constexpr Vector3 Forward = {  0.f,  0.f,  1.f };
    inline constexpr Vector3 Right   = {  1.f,  0.f,  0.f };
    inline constexpr Vector3 Up      = {  0.f,  1.f,  0.f };
}
