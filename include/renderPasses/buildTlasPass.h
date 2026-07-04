#pragma once

#include <span>

#include "renderPass.h"
#include "frameGraphBuilder.h"
#include "renderScene.h"
#include "device.h"

namespace Hydrogen
{
    class GpuScene;

    class BuildTlasPass : public IRenderPass
    {
    public:
        GpuScene* pScene = nullptr;
        std::span<const RenderObject> renderObjects{};

        void Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) override;
        void Setup(FGBuilder& builder) override;
        void Execute(FGExecuteContext& ctx, GraphicsContext& gfx) override;

        uint32 GetTlasSrvIndex() const { return m_tlasSrv.index; }

        struct TlasSizes { uint64 resultSize = 0; uint64 scratchSize = 0; };
        TlasSizes QuerySizes(uint32 instanceCount);

        FGResourceHandle m_tlasHandle{};
        FGResourceHandle m_scratchHandle{};

    private:
        GpuDevice* m_pDevice = nullptr;
        ShaderResourceViewHandle m_tlasSrv{};

        uint32 m_lastQueriedInstanceCount = std::numeric_limits<uint32>::max();
        TlasSizes m_cachedSizes{};
    };
}
