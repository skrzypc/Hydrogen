#pragma once

#include <string>

#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/gpu/pipeline/pipelineState.h>
#include <h2/core/basicTypes.h>

namespace Hydrogen
{
    class DummyPass : public IRenderPass
    {
    public:
        struct PushConstants
        {
            uint32 albedoIndex = 0;
            uint32 normalIndex = 0;
            uint32 roughnessMetallicIndex = 0;
            uint32 depthIndex = 0;
            uint32 outputIndex = 0;
            uint32 indexToOutput = 0;
        };

        std::string output = "SceneColor";
        uint32 indexToOutput = 0;

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
