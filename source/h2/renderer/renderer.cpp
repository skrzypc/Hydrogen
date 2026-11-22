
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <string_view>

#include <directx/d3d12.h>
#include <DirectXTex.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

#include <pix3.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>

#include <h2/renderer/renderer.h>
#include <h2/renderer/renderPipelines/rasterDeferredRenderPipeline.h>
#include <h2/renderer/renderPipelines/referencePathTracerRenderPipeline.h>
#include <h2/renderer/renderPipelines/realtimePathTracerRenderPipeline.h>
#include <h2/core/logger.h>
#include <h2/core/verifier.h>
#include <h2/core/utilities.h>
#include <h2/core/timer.h>
#include <h2/shaders/shaderInterop.h>

#include <h2/renderer/framegraph/frameGraph.h>
#include <h2/renderer/gpu/commands/graphicsContext.h>
#include <h2/renderer/gpu/scene/gpuScene.h>
#include <h2/assets/assetUploadQueue.h>
#include <h2/renderer/renderScene.h>
#include <h2/core/math.h>
#include <h2/renderer/frameContext.h>

namespace Hydrogen
{
    Renderer::~Renderer()
    {
        if (m_renderPipeline == nullptr)
        {
            return;
        }

        m_gpuDevice.WaitForIdle<eQueueType::Direct>();
        m_renderPipeline->Shutdown();
        m_imguiPass.Shutdown();
    }

    void Renderer::Initialize(uint32 renderWidth, uint32 renderHeight, HWND hWnd, const RuntimePaths& paths)
    {
        m_renderWidth = renderWidth;
        m_renderHeight = renderHeight;

        m_shaderCompiler.Initialize(paths.shaderSourceDir, paths.shaderOutputDir);

        m_gpuDevice.Create();
        m_swapChain.Create(m_gpuDevice, hWnd);

        m_frameGraph.Initialize(m_gpuDevice);
        m_uploadBuffer.Initialize(m_gpuDevice, 1024 * 1024); // 1 MiB per frame
        GraphicsContext::s_pUploadBuffer = &m_uploadBuffer;
        m_gpuUploader.Initialize(m_gpuDevice, static_cast<uint64>(256ull * 1024ull * 1024ull));
        m_gpuScene.Initialize(m_gpuDevice, m_gpuUploader, 50'000'000, 150'000'000);

        m_viewBuffer = m_gpuDevice.CreateUploadBuffer(L"H2_VIEW_BUFFER",
                                                      m_maxViews * sizeof(ViewData) * StaticConfig::FramesInFlight);
        D3D12_SHADER_RESOURCE_VIEW_DESC viewSrvDesc{};
        viewSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
        viewSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        viewSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewSrvDesc.Buffer.FirstElement = 0;
        viewSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        viewSrvDesc.Buffer.NumElements = m_maxViews * StaticConfig::FramesInFlight;
        viewSrvDesc.Buffer.StructureByteStride = sizeof(ViewData);
        m_viewBufferSrv = m_gpuDevice.CreateShaderResourceView(m_viewBuffer.get(), viewSrvDesc);

        m_captureDir = paths.captureDir;

        CreateRenderPipeline(m_renderPipelineType);

        m_imguiPass.Initialize(m_gpuDevice, m_shaderCompiler);
    }

    void Renderer::SubmitFrame(const Scene& scene, ImDrawData* drawData, const RendererUiRequest& uiRequest,
                               float64 time, float32 deltaTime)
    {
        if (uiRequest.renderPipelineType.has_value())
        {
            SwitchRenderPipeline(*uiRequest.renderPipelineType);
        }

        RenderScene renderScene{};

        if (const TransformComponent* pCameraTransform = scene.transforms.Get(scene.activeCamera))
        {
            renderScene.camera.position = pCameraTransform->transform.position;
            renderScene.camera.rotation = pCameraTransform->transform.rotation;
        }

        if (const CameraComponent* pCameraComponent = scene.cameras.Get(scene.activeCamera))
        {
            renderScene.camera.fovYDeg = pCameraComponent->fovYDeg;
            renderScene.camera.nearZ = pCameraComponent->nearZ;
            renderScene.camera.farZ = pCameraComponent->farZ;
            renderScene.camera.exposure = 683.0f / (1.2f * std::exp2(pCameraComponent->ev100)); // EV100 to exposure.
        }

        const auto& meshEntities = scene.meshes.GetEntities();
        const auto meshComponents = scene.meshes.GetAll();
        for (uint32 meshIndex = 0; meshIndex < static_cast<uint32>(meshEntities.size()); ++meshIndex)
        {
            const TransformComponent* pTransformComponent = scene.transforms.Get(meshEntities[meshIndex]);
            if (pTransformComponent == nullptr)
            {
                continue;
            }

            RenderObject renderObject{};
            renderObject.mesh = meshComponents[meshIndex].mesh;
            if (const MaterialComponent* pMaterialComponent = scene.materials.Get(meshEntities[meshIndex]))
            {
                renderObject.materialDataIndex = pMaterialComponent->material.id;
            }
            DirectX::XMStoreFloat4x4(&renderObject.worldMatrix, pTransformComponent->transform.GetWorldMatrix());
            renderScene.objects.push_back(renderObject);
        }

        const auto& lightEntities = scene.lights.GetEntities();
        const auto lightComponents = scene.lights.GetAll();
        for (uint32 lightIndex = 0; lightIndex < static_cast<uint32>(lightEntities.size()); ++lightIndex)
        {
            const TransformComponent* pTransformComponent = scene.transforms.Get(lightEntities[lightIndex]);
            if (pTransformComponent == nullptr)
            {
                continue;
            }

            RenderLight renderLight{};
            renderLight.light = lightComponents[lightIndex].light;
            renderLight.position = pTransformComponent->transform.position;
            renderLight.direction = Vector3::Transform(Forward, Quaternion(pTransformComponent->transform.rotation));
            renderScene.lights.push_back(renderLight);
        }

        // TODO: Translate uiRequest into some sort of render settings.
        // TODO: We should have something like FrameContext instead of passing all these parameters separately.
        RenderFrame(renderScene, uiRequest, drawData, time, deltaTime);
    }

    FrameContext Renderer::BeginFrame(const RenderScene& renderScene, float64 time, float32 deltaTime)
    {
        const uint32 frameIndex = m_swapChain.GetCurrentFrameIndex();

        m_gpuDevice.Wait<eQueueType::Direct>(m_frameFenceValues[frameIndex]);

        m_uploadBuffer.NextFrame(frameIndex);

        ProcessUploadQueue();

        const Texture::Desc& backBufferDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();

        return FrameContext{
            .gpuScene = m_gpuScene,
            .renderScene = renderScene,

            .renderWidth = backBufferDesc.width,
            .renderHeight = backBufferDesc.height,

            .displayWidth = backBufferDesc.width,
            .displayHeight = backBufferDesc.height,
            .displayFormat = backBufferDesc.format,

            .frameNumber = m_swapChain.GetCurrentFrameNumber(),
            .frameIndex = frameIndex,

            .time = time,
            .deltaTime = deltaTime,
        };
    }

    void Renderer::EndFrame(uint32 frameIndex, uint64 fenceValue)
    {
        m_frameFenceValues[frameIndex] = fenceValue;
        m_gpuScene.ClearSceneChanged();
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

        auto materialRequests = m_pUploadQueue->DrainMaterials();
        if (!materialRequests.empty())
        {
            std::vector<MaterialHandle> materialHandles{};
            materialHandles.reserve(materialRequests.size());

            std::vector<Material> materials{};
            materials.reserve(materialRequests.size());

            for (auto&& request : materialRequests)
            {
                materialHandles.push_back(request.handle);
                materials.push_back(std::move(request.material));
            }

            m_gpuScene.RegisterMaterials(materialHandles, materials);
        }

        auto textureRequests = m_pUploadQueue->DrainTextures();
        if (!textureRequests.empty())
        {
            std::vector<TextureHandle> textureHandles{};
            textureHandles.reserve(textureRequests.size());

            std::vector<TextureData> textures{};
            textures.reserve(textureRequests.size());

            for (auto&& request : textureRequests)
            {
                textureHandles.push_back(request.handle);
                textures.push_back(std::move(request.data));
            }

            m_gpuScene.RegisterTextures(textureHandles, textures);
        }
    }

    void Renderer::CreateRenderPipeline(eRenderPipelineType type)
    {
        switch (type)
        {
        case eRenderPipelineType::RasterDeferred:
            m_renderPipeline = std::make_unique<RasterDeferredRenderPipeline>();
            break;
        case eRenderPipelineType::ReferencePathTracer:
            m_renderPipeline = std::make_unique<ReferencePathTracerRenderPipeline>();
            break;
        case eRenderPipelineType::RealtimePathTracer:
            m_renderPipeline = std::make_unique<RealtimePathTracerRenderPipeline>();
            break;
        default:
            H2_VERIFY_FATAL(false, "Unknown render pipeline type!");
            break;
        }

        m_renderPipeline->Initialize(m_gpuDevice, m_shaderCompiler, m_renderWidth, m_renderHeight);
        m_renderPipelineType = type;
    }

    void Renderer::SwitchRenderPipeline(eRenderPipelineType type)
    {
        if (m_renderPipelineType == type)
        {
            return;
        }

        m_gpuDevice.WaitForIdle<eQueueType::Direct>();
        m_renderPipeline->Shutdown();
        m_renderPipeline.reset();

        CreateRenderPipeline(type);
    }

    RendererUiRequest Renderer::BuildUi()
    {
        RendererUiRequest request{};

        static constexpr std::array pipelineNames = {
            RasterDeferredRenderPipeline::Name,
            ReferencePathTracerRenderPipeline::Name,
            RealtimePathTracerRenderPipeline::Name,
        };
        static_assert(pipelineNames.size() == static_cast<size_t>(eRenderPipelineType::Count),
                      "pipelineNames must list every eRenderPipelineType");

        // ImGui needs null-terminated strings; string_view literals are, so data() is safe here.
        std::array<const char*, pipelineNames.size()> pipelineItems{};
        std::ranges::transform(pipelineNames, pipelineItems.begin(), [](std::string_view name) { return name.data(); });

        auto selected = static_cast<int32>(m_renderPipelineType);
        if (ImGui::Combo("Render pipeline", &selected, pipelineItems.data(), static_cast<int32>(pipelineItems.size())))
        {
            request.renderPipelineType = static_cast<eRenderPipelineType>(selected);
        }

        request.captureOutput = ImGui::Button("Capture output");

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const float32 framerate = ImGui::GetIO().Framerate;
            ImGui::Text("FPS: %.1f (%.2f ms)", framerate, 1000.0f / framerate);
            ImGui::Spacing();
        }

        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Pipeline settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            m_renderPipeline->BuildUI();
            ImGui::Spacing();
        }

        return request;
    }

    // Reversed-Z LH projection: near maps to 1, far maps to 0.
    static DirectX::XMMATRIX PerspectiveFovLH_ReversedZ(float fovY, float aspect, float nearZ, float farZ)
    {
        const float h = 1.0f / std::tan(fovY * 0.5f);
        const float w = h / aspect;
        const float A = -nearZ / (farZ - nearZ);
        const float B = nearZ * farZ / (farZ - nearZ);
        return DirectX::XMMATRIX(w, 0, 0, 0, 0, h, 0, 0, 0, 0, A, 1, 0, 0, B, 0);
    }

    void Renderer::UpdateFrameData(const FrameContext& frameContext)
    {
        using namespace DirectX;

        const RenderScene& renderScene = frameContext.renderScene;

        const float32 renderWidth = static_cast<float32>(frameContext.renderWidth);
        const float32 renderHeight = static_cast<float32>(frameContext.renderHeight);
        const float32 aspect = renderWidth / renderHeight;

        const Matrix view = (Matrix::CreateFromQuaternion(renderScene.camera.rotation) *
                             Matrix::CreateTranslation(renderScene.camera.position))
                                .Invert();
        const Matrix proj = PerspectiveFovLH_ReversedZ(ToRadians(renderScene.camera.fovYDeg), aspect,
                                                       renderScene.camera.nearZ, renderScene.camera.farZ);
        const Matrix vp = view * proj;

        ViewData viewData{};
        XMStoreFloat4x4(&viewData.viewMx, view);
        XMStoreFloat4x4(&viewData.projectionMx, proj);
        XMStoreFloat4x4(&viewData.viewProjectionMx, vp);
        XMStoreFloat4x4(&viewData.invViewProjectionMx, vp.Invert());
        viewData.worldPosition = renderScene.camera.position;
        viewData.worldDirection = Vector3::Transform(Forward, renderScene.camera.rotation);
        viewData.nearPlane = renderScene.camera.nearZ;
        viewData.farPlane = renderScene.camera.farZ;
        viewData.viewportSize = {renderWidth, renderHeight};
        viewData.exposure = renderScene.camera.exposure;

        const uint32 viewDataIndex = 0;
        const uint32 viewDataSlotIndex = m_maxViews * frameContext.frameIndex + viewDataIndex;
        const uint32 viewDataSlotOffset = (sizeof(ViewData) * viewDataSlotIndex);
        m_viewBuffer->Write(&viewData, sizeof(ViewData), viewDataSlotOffset);

        FrameData frameData{};
        frameData.viewBufferIndex = m_viewBufferSrv.index;
        frameData.mainViewIndex = viewDataSlotIndex;
        frameData.vertexPositionBufferIndex = m_gpuScene.GetPositionBufferIndex();
        frameData.vertexNormalBufferIndex = m_gpuScene.GetNormalBufferIndex();
        frameData.vertexTangentBufferIndex = m_gpuScene.GetTangentBufferIndex();
        frameData.vertexUvBufferIndex = m_gpuScene.GetUvBufferIndex();
        frameData.indexBufferIndex = m_gpuScene.GetIndexBufferIndex();
        frameData.meshDataBufferIndex = m_gpuScene.GetMeshDataBufferIndex();
        frameData.instanceDataBufferIndex = m_gpuScene.GetInstanceDataBufferIndex();
        frameData.transformBufferIndex = m_gpuScene.GetTransformBufferIndex();
        frameData.materialDataBufferIndex = m_gpuScene.GetMaterialDataBufferIndex();
        frameData.lightBufferIndex = m_gpuScene.GetLightBufferIndex();
        frameData.lightCount = m_gpuScene.GetLightCount();
        frameData.time = static_cast<float32>(frameContext.time);
        frameData.deltaTime = frameContext.deltaTime;
        frameData.frameNumber = static_cast<uint32>(frameContext.frameNumber);
        m_renderPipeline->FillFrameData(frameData);

        auto [pCpu, gpuAddr] = m_uploadBuffer.Allocate(sizeof(FrameData));
        memcpy(pCpu, &frameData, sizeof(FrameData));
        GraphicsContext::s_frameDataAddr = gpuAddr;
    }

    void Renderer::PrepareFrameCaptureTarget(const FrameContext& frameContext)
    {
        // Filled with CopyResource, so it must match the pipeline output's size and format exactly.
        if (m_frameCaptureTarget != nullptr)
        {
            const Texture::Desc& current = m_frameCaptureTarget->GetDesc();
            if (current.width == frameContext.displayWidth && current.height == frameContext.displayHeight &&
                current.format == frameContext.displayFormat)
            {
                return;
            }
        }

        // Safe to replace: captures block until the GPU is done, so no frame still uses the old texture.
        m_frameCaptureTarget = m_gpuDevice.CreateTexture(L"H2_FRAME_CAPTURE_TARGET",
                                                         Texture::Desc{
                                                             .width = frameContext.displayWidth,
                                                             .height = frameContext.displayHeight,
                                                             .mipLevels = 1,
                                                             .arraySize = 1,
                                                             .format = frameContext.displayFormat,
                                                             .flags = D3D12_RESOURCE_FLAG_NONE,
                                                             .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                         },
                                                         ResourceState{.layout = D3D12_BARRIER_LAYOUT_COMMON});
    }

    void Renderer::SaveFrameCapture()
    {
        DirectX::ScratchImage image{};
        if (!H2_VERIFY(DirectX::CaptureTexture(m_gpuDevice.GetDxQueue<eQueueType::Direct>(),
                                               m_frameCaptureTarget->GetResource(), false, image,
                                               D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON),
                       "Failed to capture the frame output!"))
        {
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(m_captureDir, error);
        if (!H2_VERIFY(!error, "Failed to create the capture directory {}: {}", m_captureDir.string(), error.message()))
        {
            return;
        }

        std::string timestamp = std::format("{:%Y-%m-%d_%H-%M-%S}", Timer::GetTime());
        std::ranges::replace(timestamp, '.', '-');
        const std::filesystem::path filePath =
            m_captureDir / std::format("{}_{}.dds", m_renderPipeline->GetName(), timestamp);

        if (H2_VERIFY(DirectX::SaveToDDSFile(*image.GetImage(0, 0, 0), DirectX::DDS_FLAGS_NONE, filePath.c_str()),
                      "Failed to save the frame capture to {}!", filePath.string()))
        {
            H2_INFO(eLogLevel::Regular, "Saved frame capture to {}", filePath.string());
        }
    }

    void Renderer::RenderFrame(const RenderScene& renderScene, const RendererUiRequest& uiRequest, ImDrawData* drawData,
                               float64 time, float32 deltaTime)
    {
        FrameContext frameContext = BeginFrame(renderScene, time, deltaTime);

        m_gpuScene.Update(frameContext);

        frameContext.sceneChanged =
            (m_gpuScene.HasSceneChanged() || frameContext.renderScene.camera.fovYDeg != m_previousCameraData.fovYDeg ||
             frameContext.renderScene.camera.nearZ != m_previousCameraData.nearZ ||
             frameContext.renderScene.camera.farZ != m_previousCameraData.farZ ||
             frameContext.renderScene.camera.position != m_previousCameraData.position ||
             frameContext.renderScene.camera.rotation != m_previousCameraData.rotation);

        m_previousCameraData = frameContext.renderScene.camera;

        UpdateFrameData(frameContext);

        m_frameGraph.BeginFrame(frameContext.frameNumber);

        m_frameGraph.ImportTexture("Backbuffer", m_swapChain.GetCurrentBackBuffer());

        std::string_view pipelineOutput = m_renderPipeline->Render(m_frameGraph, frameContext);

        if (uiRequest.captureOutput)
        {
            PrepareFrameCaptureTarget(frameContext);
            m_frameGraph.ImportTexture("FrameCaptureTarget", m_frameCaptureTarget.get());

            m_copyPassSaveOutput.input = pipelineOutput;
            m_copyPassSaveOutput.output = "FrameCaptureTarget";
            m_frameGraph.AddPass("CopyToFrameCaptureTarget", m_copyPassSaveOutput);
        }

        if (drawData != nullptr)
        {
            m_imguiPass.pDrawData = drawData;
            m_imguiPass.target = pipelineOutput;
            m_frameGraph.AddPass("ImGui", m_imguiPass);
        }

        m_copyPass.input = pipelineOutput;
        m_copyPass.output = "Backbuffer";
        m_frameGraph.AddPass("CopyToBackbuffer", m_copyPass);

        m_frameGraph.Compile();

        GraphicsContext gfx = m_frameGraph.Execute();
        uint64 fenceValue = m_gpuDevice.ExecuteGraphicsContext(std::move(gfx));

        if (uiRequest.captureOutput)
        {
            SaveFrameCapture();
        }

        m_frameGraph.Reset();

        EndFrame(frameContext.frameIndex, fenceValue);
    }
} // namespace Hydrogen
