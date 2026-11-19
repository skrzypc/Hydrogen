#pragma once

#include <string>

#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>
#include <h2/renderer/gpu/pipeline/pipelineState.h>
#include <h2/renderer/gpu/device.h>

namespace Hydrogen
{
    class ReSTIRShadingPass : public IRenderPass
    {
    public:
        struct PushConstants
        {
            uint32 tlasIndex;
            uint32 albedoIndex;
            uint32 normalIndex;
            uint32 roughnessMetallicIndex;
            uint32 depthIndex;
            uint32 reservoirIndex;
            uint32 outputIndex;
        };

        std::string reservoirs = "ReSTIR_InitialReservoirs";
        std::string output = "SceneColor";

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

    private:
        static constexpr uint32 ThreadGroupSize = 8;

        GpuDevice* m_pDevice = nullptr;

        PipelineState m_pso{};

        uint32 m_width = 0;
        uint32 m_height = 0;
    };
} // namespace Hydrogen
