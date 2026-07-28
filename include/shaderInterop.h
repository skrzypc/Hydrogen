#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>
#include "basicTypes.h"

namespace Hydrogen
{
	using float2 = DirectX::XMFLOAT2;
	using float3 = DirectX::XMFLOAT3;
	using float4x4 = DirectX::XMFLOAT4X4;
	using uint2 = DirectX::XMUINT2;
	using uint = uint32;
#endif

	struct FrameData
	{
		uint vertexPositionBufferIndex;
		uint vertexNormalBufferIndex;
		uint vertexUvBufferIndex;
		uint transformBufferIndex;

		uint outputTargetUavIndex;

		uint viewBufferIndex;
		uint mainViewIndex;

		uint tlasIndex;

		float time;
		float deltaTime;
		uint frameNumber;
	};

	struct ViewData
	{
		float4x4 viewMx;
		float4x4 projectionMx;
		float4x4 viewProjectionMx;
		float4x4 invViewProjectionMx;

		float3 worldPosition;
		float nearPlane;

		float3 worldDirection;
		float farPlane;

		float2 viewportSize;
		uint2 _pad;
	};

#ifdef __cplusplus
} // namespace Hydrogen
#endif
