
#include <cmath>

#include <d3d12.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

#include <pix3.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"
#include "shaderInterop.h"

#include "frameGraph.h"
#include "graphicsContext.h"
#include "gpuScene.h"
#include "assetUploadQueue.h"
#include "renderScene.h"
#include "hydrogenMath.h"

namespace Hydrogen
{
	Renderer::~Renderer()
	{
		m_gpuDevice.WaitForIdle<eQueueType::Direct>();
		m_imguiPass.Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
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
		m_clearPass.clearColor = { 0.53f, 0.81f, 0.92f, 1.0f };
		m_meshPass.Initialize(m_gpuDevice, m_shaderCompiler);

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(hWnd);
		m_imguiPass.Initialize(m_gpuDevice, m_shaderCompiler);
	}

	void Renderer::BeginFrame()
	{
		m_currentFrameIndex = m_swapChain.GetCurrentFrameIndex();
		m_uploadBuffer.NextFrame(m_currentFrameIndex);
		m_gpuDevice.Wait<eQueueType::Direct>(m_frameFenceValues[m_currentFrameIndex]);
	}

	void Renderer::EndFrame(uint32 frameIndex, uint64 fenceValue)
	{
		m_frameFenceValues[frameIndex] = fenceValue;
		m_swapChain.Present();
	}

	void Renderer::ProcessUploadQueue()
	{
		if (!m_pUploadQueue)
		{
			return;
		}

		auto requests = m_pUploadQueue->Drain();
		if (!requests.empty())
		{
			// TODO: Requests should be SoAs.
			std::vector<MeshHandle> meshHandles{};
			meshHandles.reserve(requests.size());

			std::vector<Mesh> meshes{};
			meshes.reserve(requests.size());

			for (auto&& request : requests)
			{
				meshHandles.push_back(request.handle);
				meshes.emplace_back(std::move(request.mesh));
			}

			uint64 fence = m_gpuScene.RegisterMeshes(meshHandles, meshes);

			// TODO: We don't want to wait
			m_gpuDevice.WaitOnQueue<eQueueType::Direct, eQueueType::Copy>(fence);
		}
	}

	// Reversed-Z LH projection: near maps to 1, far maps to 0.
	static DirectX::XMMATRIX PerspectiveFovLH_ReversedZ(float fovY, float aspect, float nearZ, float farZ)
	{
		const float h = 1.0f / std::tan(fovY * 0.5f);
		const float w = h / aspect;
		const float A = -nearZ / (farZ - nearZ);
		const float B = nearZ * farZ / (farZ - nearZ);
		return DirectX::XMMATRIX(
			w,  0,  0,  0,
			0,  h,  0,  0,
			0,  0,  A,  1,
			0,  0,  B,  0
		);
	}

	void Renderer::UpdateFrameData(const RenderScene& renderScene)
	{
		using namespace DirectX;

		const Texture::Desc& bbDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();
		const float32 aspect = static_cast<float32>(bbDesc.width) / static_cast<float32>(bbDesc.height);

		const Matrix view = (Matrix::CreateFromQuaternion(renderScene.camera.rotation) *
			Matrix::CreateTranslation(renderScene.camera.position)).Invert();
		const Matrix proj = PerspectiveFovLH_ReversedZ(
			ToRadians(renderScene.camera.fovYDeg), aspect,
			renderScene.camera.nearZ, renderScene.camera.farZ);
		const Matrix vp = view * proj;

		ShaderInterop::ViewData viewData{};
		XMStoreFloat4x4(&viewData.viewMx, view);
		XMStoreFloat4x4(&viewData.projectionMx, proj);
		XMStoreFloat4x4(&viewData.viewProjectionMx, vp);
		XMStoreFloat4x4(&viewData.invViewProjectionMx, vp.Invert());
		viewData.nearPlane = renderScene.camera.nearZ;
		viewData.farPlane = renderScene.camera.farZ;
		viewData.viewportSize = { static_cast<float32>(bbDesc.width), static_cast<float32>(bbDesc.height) };
		m_gpuScene.UpdateView(viewData);

		// Upload transforms
		std::vector<DirectX::XMFLOAT4X4> matrices;
		matrices.reserve(renderScene.objects.size());
		for (const RenderObject& obj : renderScene.objects)
		{
			matrices.push_back(obj.worldMatrix);
		}
		m_gpuScene.UpdateTransforms(matrices);

		ShaderInterop::FrameData frameData{};
		frameData.viewBufferIndex = m_gpuScene.GetViewBufferSrv().index;
		frameData.mainViewIndex = 0;
		frameData.vertexPositionBufferIndex = m_gpuScene.GetPositionSrv().index;
		frameData.vertexNormalBufferIndex = m_gpuScene.GetNormalSrv().index;
		frameData.vertexUvBufferIndex = m_gpuScene.GetUvSrv().index;
		frameData.transformBufferIndex = m_gpuScene.GetTransformBufferSrv().index;
		frameData.time = m_time;
		frameData.frameNumber = static_cast<uint32>(m_swapChain.GetCurrentFrameNumber());

		auto [pCpu, gpuAddr] = m_uploadBuffer.Allocate(sizeof(ShaderInterop::FrameData));
		memcpy(pCpu, &frameData, sizeof(ShaderInterop::FrameData));
		GraphicsContext::s_frameDataAddr = gpuAddr;
	}

	void Renderer::RenderFrame(const RenderScene& renderScene, ImDrawData* drawData)
	{
		uint64 currentFrameNumber = m_swapChain.GetCurrentFrameNumber();

		BeginFrame();

		ProcessUploadQueue();
		UpdateFrameData(renderScene);

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
					.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.optimizedClearColor = m_clearPass.clearColor,
				}
				);

			m_frameGraph.CreateTexture("SceneDepth",
				{
					.width = backBufferDesc.width,
					.height = backBufferDesc.height,
					.mipLevels = 1,
					.arraySize = 1,
					.format = DXGI_FORMAT_D32_FLOAT,
					.flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
					.dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					.optimizedDepthClearValue = 0.0f,
				}
			);
		}

		m_clearPass.target = "DefaultTarget";
		m_frameGraph.AddPass("ClearTarget", m_clearPass);

		m_meshPass.target = "DefaultTarget";
		m_meshPass.depthTarget = "SceneDepth";
		m_meshPass.pScene = &m_gpuScene;
		m_meshPass.renderObjects = renderScene.objects;
		m_frameGraph.AddPass("MeshPass", m_meshPass);

		m_imguiPass.pDrawData = drawData;
		m_imguiPass.target = "DefaultTarget";
		m_frameGraph.AddPass("ImGui", m_imguiPass);

		m_copyPass.src = "DefaultTarget";
		m_copyPass.dst = "Backbuffer";
		m_frameGraph.AddPass("CopyToBackbuffer", m_copyPass);

		m_frameGraph.Compile();
		GraphicsContext gfx = m_frameGraph.Execute();
		m_frameGraph.Reset();

		uint64 fenceValue = m_gpuDevice.ExecuteGraphicsContext(std::move(gfx));

		EndFrame(m_currentFrameIndex, fenceValue);
	}
}
