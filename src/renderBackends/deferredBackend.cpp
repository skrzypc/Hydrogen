#include "renderBackends/deferredBackend.h"

#include <string_view>

#include <d3d12.h>

#include "frameGraph.h"
#include "gpuScene.h"
#include "shaderInterop.h"

namespace Hydrogen
{
	void DeferredBackend::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
	{
		m_pDevice = &device;

		m_gBufferPass.Initialize(device, shaderCompiler);
		m_lightingPass.Initialize(device, shaderCompiler);
		m_tonemapPass.Initialize(device, shaderCompiler);
	}

	void DeferredBackend::Shutdown() { }

	void DeferredBackend::DefineFrameGraphResources(FrameGraph& frameGraph, const FrameContext& frameContext)
	{
		frameGraph.CreateTexture("GBuffer_Albedo",
			{
				.width = frameContext.renderWidth,
				.height = frameContext.renderHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = GBufferPass::AlbedoFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
			});

		frameGraph.CreateTexture("GBuffer_Normal",
			{
				.width = frameContext.renderWidth,
				.height = frameContext.renderHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = GBufferPass::NormalFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedClearColor = {0.0f, 0.0f},
			});

		frameGraph.CreateTexture("GBuffer_RM",
			{
				.width = frameContext.renderWidth,
				.height = frameContext.renderHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = GBufferPass::RoughnessMetalnessFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedClearColor = {0.0f, 0.0f},
			});

		frameGraph.CreateTexture("SceneColor",
			{
				.width = frameContext.renderWidth,
				.height = frameContext.renderHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = LightingPass::SceneColorFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
			});

		frameGraph.CreateTexture("Output",
			{
				.width = frameContext.displayWidth,
				.height = frameContext.displayHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = frameContext.displayFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedClearColor = {0.0f, 0.0f, 0.0f, 1.0f},
			});

		frameGraph.CreateTexture("SceneDepth",
			{
				.width = frameContext.renderWidth,
				.height = frameContext.renderHeight,
				.mipLevels = 1,
				.arraySize = 1,
				.format = GBufferPass::DepthFormat,
				.flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.optimizedDepthClearValue = 0.0f,
			});
	}

	std::string_view DeferredBackend::Render(FrameGraph& frameGraph, const FrameContext& frameContext)
	{
		DefineFrameGraphResources(frameGraph, frameContext);

		m_gBufferPass.pScene = &frameContext.gpuScene;
		m_gBufferPass.renderObjects = frameContext.renderScene.objects;
		frameGraph.AddPass("GBuffer", m_gBufferPass);

		frameGraph.AddPass("Lighting", m_lightingPass);

		frameGraph.AddPass("Tonemap", m_tonemapPass);

		return "Output";
	}

	void DeferredBackend::BuildUI() { }
}
