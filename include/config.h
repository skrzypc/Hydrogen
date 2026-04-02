#pragma once

#include "basicTypes.h"

#include "logger.h"
#include "device.h"

namespace Hydrogen
{
	struct Config
	{
		static constexpr bool WaitForDebugger = false;

		static constexpr uint32 FramesInFlight = 3;

		static constexpr uint32 WindowWidth = 1920;
		static constexpr uint32 WindowHeight = 1080;

		static constexpr eLogLevel LogLevel = eLogLevel::Verbose;

		static constexpr bool EnableGPUBasedValidation = false;
		static constexpr eAdapterVendor RequestedAdapter = eAdapterVendor::NVIDIA;

		static constexpr D3D12_RAYTRACING_TIER ExpectedRaytracingTier = D3D12_RAYTRACING_TIER_1_1; // 1_2 not supported by AMD yet.
		static constexpr D3D12_MESH_SHADER_TIER ExpectedMeshShaderTier = D3D12_MESH_SHADER_TIER_1;
		static constexpr D3D12_RESOURCE_BINDING_TIER ExpectedResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_3;

		static constexpr uint8 ShaderModelVersionMajor = 6;
		static constexpr uint8 ShaderModelVersionMinor = 9;
	};
}