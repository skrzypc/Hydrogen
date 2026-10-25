#pragma once

#include <string>

#include <h2/renderer/passes/renderPass.h>
#include <h2/renderer/framegraph/frameGraphStructs.h>
#include <h2/renderer/gpu/descriptors/indexAllocators.h>
#include <h2/renderer/gpu/descriptors/descriptorHeap.h>

struct ImDrawData;

namespace Hydrogen
{
    class ImguiPass : public IRenderPass
    {
    public:
        std::string target;
        ImDrawData* pDrawData = nullptr;

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext) override;

        void Shutdown();

    private:
        struct HeapSlotAllocator
        {
            const DescriptorHeap* descHeap = nullptr;
            FreeListIndexAllocator descAllocator{};
        };

        HeapSlotAllocator m_descriptorBundle{};
    };
} // namespace Hydrogen
