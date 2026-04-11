#pragma once

#include <DirectXMath.h>

#include "basicTypes.h"

namespace Hydrogen::ShaderInterop
{
	struct FrameData
	{
		uint32 transformBufferIndex = 0u;

		float32 time = 0.0f;
		float32 deltaTime = 0.0f;
		uint32 frameNumber = 0u; // 32bit enough?

		//uint32 _pad = 0u;
	};

	//struct ViewData
	//{
	//	DirectX::XMFLOAT4X4 viewMx{};
	//	DirectX::XMFLOAT4X4 projectionMx{};
	//	DirectX::XMFLOAT4X4 viewProjectionMx{};
	//	DirectX::XMFLOAT4X4 invViewProjectionMx{};

	//	DirectX::XMFLOAT3 worldPosition{};
	//	float32 nearPlane = 0.0f;

	//	DirectX::XMFLOAT3 worldDirection{};
	//	float32 farPlane = 0.0f;

	//	DirectX::XMFLOAT2 viewportSize{};
	//	uint32 _pad0 = 0u;
	//	uint32 _pad1 = 0u;
	//};
}
