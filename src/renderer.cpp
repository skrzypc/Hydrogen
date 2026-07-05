
#include <cmath>
#include <string_view>

#include <d3d12.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

#include <pix3.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include "renderer.h"
#include "renderBackends/hybridBackend.h"
#include "renderBackends/pathTracerBackend.h"
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
		m_backend->Shutdown();
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
		m_gpuScene.Initialize(m_gpuDevice, m_gpuUploader, 10'000'000, 30'000'000);

		m_viewBuffer = m_gpuDevice.CreateUploadBuffer(L"H2_VIEW_BUFFER", m_maxViews * sizeof(ShaderInterop::ViewData));
		D3D12_SHADER_RESOURCE_VIEW_DESC viewSrvDesc{};
		viewSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewSrvDesc.Buffer.FirstElement = 0;
		viewSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		viewSrvDesc.Buffer.NumElements = m_maxViews;
		viewSrvDesc.Buffer.StructureByteStride = sizeof(ShaderInterop::ViewData);
		m_viewBufferSrv = m_gpuDevice.CreateShaderResourceView(m_viewBuffer.get(), viewSrvDesc);

		CreateBackend(eRenderBackendType::Hybrid);

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

			m_gpuScene.RegisterMeshes(meshHandles, meshes);
		}
	}

	void Renderer::CreateBackend(eRenderBackendType type)
	{
		switch (type)
		{
		case eRenderBackendType::Hybrid:
			m_backend = std::make_unique<HybridBackend>();
			break;
		case eRenderBackendType::PathTracer:
			m_backend = std::make_unique<PathTracerBackend>();
			break;
		default:
			H2_VERIFY_FATAL(false, "Unknown render backend type!");
			break;
		}

		m_backend->Initialize(m_gpuDevice, m_shaderCompiler, m_gpuScene);
		m_backendType = type;
	}

	void Renderer::SwitchBackend(eRenderBackendType type)
	{
		if (m_backendType == type)
		{
			return;
		}

		m_gpuDevice.WaitForIdle<eQueueType::Direct>();
		m_backend->Shutdown();
		m_backend.reset();

		CreateBackend(type);
	}

	void Renderer::BuildBackendUI()
	{
		static constexpr std::array<const char*, static_cast<size_t>(eRenderBackendType::Count)> backendNames =
		{
			"Hybrid",
			"PathTracer",
		};

		ImGui::Begin("Renderer");

		int selected = static_cast<int>(m_backendType);
		if (ImGui::Combo("Backend", &selected, backendNames.data(), static_cast<int>(backendNames.size())))
		{
			SwitchBackend(static_cast<eRenderBackendType>(selected));
		}

		ImGui::End();

		m_backend->BuildUI();
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
		m_viewBuffer->Write(&viewData, sizeof(ShaderInterop::ViewData), 0);

		const SceneBindings bindings = m_gpuScene.Update(renderScene, m_currentFrameIndex);

		ShaderInterop::FrameData frameData{};
		frameData.viewBufferIndex = m_viewBufferSrv.index;
		frameData.mainViewIndex = 0;
		frameData.vertexPositionBufferIndex = bindings.positionBufferIndex;
		frameData.vertexNormalBufferIndex = bindings.normalBufferIndex;
		frameData.vertexUvBufferIndex = bindings.uvBufferIndex;
		frameData.transformBufferIndex = bindings.transformBufferIndex;
		frameData.time = m_time;
		frameData.frameNumber = static_cast<uint32>(m_swapChain.GetCurrentFrameNumber());
		m_backend->FillFrameData(frameData);

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

		m_frameGraph.ImportTexture("Backbuffer", m_swapChain.GetCurrentBackBuffer());
		const Texture::Desc& backBufferDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();

		std::string_view backendOutput = m_backend->Render(m_frameGraph, renderScene, backBufferDesc);

		m_imguiPass.pDrawData = drawData;
		m_imguiPass.target = backendOutput;
		m_frameGraph.AddPass("ImGui", m_imguiPass);

		m_copyPass.src = backendOutput;
		m_copyPass.dst = "Backbuffer";
		m_frameGraph.AddPass("CopyToBackbuffer", m_copyPass);

		m_frameGraph.Compile();
		GraphicsContext gfx = m_frameGraph.Execute();
		m_frameGraph.Reset();

		uint64 fenceValue = m_gpuDevice.ExecuteGraphicsContext(std::move(gfx));

		EndFrame(m_currentFrameIndex, fenceValue);
	}
}
