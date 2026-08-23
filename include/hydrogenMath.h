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
	inline constexpr Vector3 Forward = { 0.f,  0.f,  1.f };
	inline constexpr Vector3 Right = { 1.f,  0.f,  0.f };
	inline constexpr Vector3 Up = { 0.f,  1.f,  0.f };

	// Wraps to (-180, 180].
	inline float32 WrapDegrees(float32 degrees)
	{
		degrees = std::fmod(degrees + 180.0f, 360.0f);
		if (degrees < 0.0f)
		{
			degrees += 360.0f;
		}
		return degrees - 180.0f;
	}

	inline bool QuaternionEquals(const DirectX::XMFLOAT4& a, const DirectX::XMFLOAT4& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}
}
