
#include <d3d12.h>
#include <DirectXColors.h>

#include <pix3.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

#include "frameGraph.h"
#include "graphicsContext.h"
#include "meshLoader.h"

namespace Hydrogen
{
	Renderer::~Renderer()
	{
		m_gpuDevice.GetDirectCommandQueue().WaitForIdle();
	}

	void Renderer::Initialize(HWND hWnd)
	{
		m_gpuDevice.Create();
		m_swapChain.Create(m_gpuDevice, hWnd);

		m_frameGraph.Initialize(m_gpuDevice);
		m_uploadBuffer.Initialize(m_gpuDevice, 1024 * 1024); // 1 MiB per frame
		GraphicsContext::s_pUploadBuffer = &m_uploadBuffer;
		m_gpuUploader.Initialize(m_gpuDevice, 64 * 1024 * 1024); // 64 MiB staging

		m_clearPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_animateBackgroundPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_overlappingRectsPass.Initialize(m_gpuDevice, m_shaderCompiler);

		// Test
		//auto meshes = MeshLoader::Load("data/models/stanfordBunny/bunny.obj");
	}

	void Renderer::BeginFrame(uint32 frameIndex)
	{
		m_uploadBuffer.NextFrame(frameIndex);
		m_gpuDevice.GetDirectCommandQueue().Wait(m_frameFenceValues[frameIndex]);
	}

	void Renderer::EndFrame(uint32 frameIndex, uint64 fenceValue)
	{
		m_frameFenceValues[frameIndex] = fenceValue;
		m_swapChain.Present();
	}

	void Renderer::RenderFrame()
	{
		uint64 currentFrameNumber = m_swapChain.GetCurrentFrameNumber();
		uint32 currentFrameIndex = m_swapChain.GetCurrentFrameIndex();

		H2_INFO(eLogLevel::Minimal, "Current frame: {}", currentFrameNumber);

		BeginFrame(currentFrameIndex);

		m_frameGraph.BeginFrame(currentFrameNumber);

		// Define all frame resources.
		{
			m_frameGraph.ImportTexture("Backbuffer", m_swapChain.GetCurrentBackBuffer());
			const Texture::Desc& backBufferDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();

			m_frameGraph.CreateTexture("DefaultTarget",
				{
					.width = backBufferDesc.width,
					.height = backBufferDesc.height,
					.mipLevels = 1,
					.arraySize = 1,
					.format = backBufferDesc.format,
					.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
					.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D
				}
			);
		}

		m_clearPass.target = "DefaultTarget";
		m_frameGraph.AddPass("ClearTarget", m_clearPass);

		m_animateBackgroundPass.target = "DefaultTarget";
		m_frameGraph.AddPass("AnimateBackground", m_animateBackgroundPass);

		m_overlappingRectsPass.target = "DefaultTarget";
		m_frameGraph.AddPass("OverlappingRects", m_overlappingRectsPass);

		m_copyPass.src = "DefaultTarget";
		m_copyPass.dst = "Backbuffer";
		m_frameGraph.AddPass("CopyToBackbuffer", m_copyPass);

		m_frameGraph.Compile();
		GraphicsContext gfx = m_frameGraph.Execute();
		m_frameGraph.Reset();

		uint64 fenceValue = m_gpuDevice.ExecuteGraphicsContext(std::move(gfx));

		EndFrame(currentFrameIndex, fenceValue);
	}
}
