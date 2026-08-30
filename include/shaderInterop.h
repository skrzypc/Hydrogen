
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
		uint indexBufferIndex;

		uint meshDataBufferIndex;
		uint instanceDataBufferIndex;
		uint transformBufferIndex;
		uint materialDataBufferIndex;

		uint viewBufferIndex;
		uint mainViewIndex;

		uint lightBufferIndex;
		uint lightCount;

		float time;
		float deltaTime;
		uint frameNumber;
	};

	struct GpuMeshData
	{
		uint baseVertex;
		uint vertexCount;
		uint baseIndex;
		uint indexCount;
	};

	struct GpuInstanceData
	{
		uint meshDataIndex;
		uint transformIndex;
		uint materialDataIndex;
		uint _pad;
	};

	struct GpuMaterialData
	{
		float3 baseColor;
		float roughness;

		float3 emissive;
		float metallic;
	};

	// Must match eLightType. Duplicated because light.h is not shader safe.
	static const uint LightTypeDirectional = 0;
	static const uint LightTypePoint = 1;
	static const uint LightTypeSpot = 2;

	struct GpuLight
	{
		float3 position;
		uint type;

		float3 color;
		// Candela (lm/sr) for point and spot, lux (lm/m^2) for directional.
		float intensity;

		float3 direction;
		// Infinite when the light has no authored range.
		float range;

		// Cosines so shading compares against a dot product directly. Spot only.
		float cosInnerConeAngle;
		float cosOuterConeAngle;
		uint2 _pad;
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
		float exposure;
		uint _pad;
	};

#ifdef __cplusplus
} // namespace Hydrogen
#endif
