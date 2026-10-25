#pragma once

#include <array>
#include <span>
#include <string>
#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/gpu/pipeline/pipelineState.h>
#include <h2/renderer/framegraph/frameGraphStructs.h>
#include <h2/renderer/renderScene.h>
#include <h2/core/basicTypes.h>

namespace Hydrogen
{
    class GpuScene;

    class GBufferPass : public IRenderPass
    {
    public:
        // The pass owns its output contract, the renderer creates the targets to match.
        static constexpr DXGI_FORMAT AlbedoFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        static constexpr DXGI_FORMAT NormalFormat = DXGI_FORMAT_R16G16_FLOAT; // Octahedral encoded.
        static constexpr DXGI_FORMAT RoughnessMetallicFormat = DXGI_FORMAT_R16G16_UNORM;
        static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;

        struct PushConstants
        {
            uint32 transformIndex = 0;
            uint32 materialIndex = 0;
            uint32 baseVertex = 0;
        };

        const GpuScene* pScene = nullptr;
        std::span<const RenderObject> renderObjects{};

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

    private:
        PipelineState m_pso{};
        uint32 m_width = 0;
        uint32 m_height = 0;
    };
} // namespace Hydrogen
