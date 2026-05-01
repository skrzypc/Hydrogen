
#include <d3d12.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

#include <pix3.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"
#include "shaderInterop.h"

#include "frameGraph.h"
#include "graphicsContext.h"
#include "meshLoader.h"
#include "gpuScene.h"

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
		m_gpuUploader.Initialize(m_gpuDevice, 256 * 1024 * 1024);
		m_gpuScene.Initialize(m_gpuDevice, m_gpuUploader, 5'000'000, 15'000'000);

		m_clearPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_animateBackgroundPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_overlappingRectsPass.Initialize(m_gpuDevice, m_shaderCompiler);
		m_meshPass.Initialize(m_gpuDevice, m_shaderCompiler);

		auto meshes = MeshLoader::Load("data/models/stanfordBunny/bunny.obj");
		auto [gpuMeshes, uploadFence] = m_gpuScene.AddMeshes(meshes);
		m_gpuDevice.GetDirectCommandQueue().WaitOnQueue(m_gpuDevice.GetCopyCommandQueue(), uploadFence);
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

	void Renderer::UpdateFrameData()
	{
		using namespace DirectX;

		const Texture::Desc& bbDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();
		const float32 aspect = static_cast<float32>(bbDesc.width) / static_cast<float32>(bbDesc.height);
		constexpr float32 kNear = 0.01f;
		constexpr float32 kFar  = 100.0f;

		XMMATRIX view = XMMatrixLookAtRH(
			XMVectorSet(0.0f, 0.1f, 0.4f, 1.0f),
			XMVectorSet(0.0f, 0.1f, 0.0f, 1.0f),
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		);
		XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(45.0f), aspect, kNear, kFar);
		XMMATRIX vp   = view * proj;

		ShaderInterop::ViewData viewData{};
		XMStoreFloat4x4(&viewData.viewMx,             view);
		XMStoreFloat4x4(&viewData.projectionMx,       proj);
		XMStoreFloat4x4(&viewData.viewProjectionMx,   vp);
		XMStoreFloat4x4(&viewData.invViewProjectionMx, XMMatrixInverse(nullptr, vp));
		viewData.nearPlane   = kNear;
		viewData.farPlane    = kFar;
		viewData.viewportSize = { static_cast<float32>(bbDesc.width), static_cast<float32>(bbDesc.height) };
		m_gpuScene.UpdateView(viewData);

		ShaderInterop::FrameData frameData{};
		frameData.viewBufferIndex     = m_gpuScene.GetViewBufferSrv().index;
		frameData.mainViewIndex       = 0;
		frameData.vertexPositionBufferIndex = m_gpuScene.GetPositionSrv().index;
		frameData.vertexNormalBufferIndex = m_gpuScene.GetNormalSrv().index;
		frameData.vertexUvBufferIndex = m_gpuScene.GetUvSrv().index;
		frameData.time            = m_time;
		frameData.frameNumber     = static_cast<uint32>(m_swapChain.GetCurrentFrameNumber());

		auto [pCpu, gpuAddr] = m_uploadBuffer.Allocate(sizeof(ShaderInterop::FrameData));
		memcpy(pCpu, &frameData, sizeof(ShaderInterop::FrameData));
		GraphicsContext::s_frameDataAddr = gpuAddr;
	}

	void Renderer::RenderFrame()
	{
		uint64 currentFrameNumber = m_swapChain.GetCurrentFrameNumber();
		uint32 currentFrameIndex = m_swapChain.GetCurrentFrameIndex();

		//H2_INFO(eLogLevel::Minimal, "Current frame: {}", currentFrameNumber);

		BeginFrame(currentFrameIndex);

		UpdateFrameData();

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

		m_meshPass.target = "DefaultTarget";
		m_meshPass.pScene = &m_gpuScene;
		m_frameGraph.AddPass("MeshPass", m_meshPass);

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
