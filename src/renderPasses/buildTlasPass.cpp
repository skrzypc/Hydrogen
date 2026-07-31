#include <d3d12.h>

#include "device.h"
#include "gpuScene.h"
#include "graphicsContext.h"
#include "frameGraphBuilder.h"
#include "renderPasses/buildTlasPass.h"

namespace Hydrogen
{
    void BuildTlasPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler)
    {
    }

    void BuildTlasPass::Setup(FGBuilder& builder)
    {
        builder.Write("TLAS", FGAccess::Write::AccelerationStructure);
        builder.Write("TLASScratch", FGAccess::Write::UnorderedAccess);
    }

    void BuildTlasPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
    {
        ID3D12Resource* pTlasResource = ctx.GetResource("TLAS");
        ID3D12Resource* pScratchResource = ctx.GetResource("TLASScratch");

        const uint32 instanceCount = pScene->GetInstanceCount();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs{};
        tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        tlasInputs.NumDescs = instanceCount;
        tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        tlasInputs.InstanceDescs = pScene->GetInstanceDescs()->GetGpuAddress();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
        buildDesc.Inputs = tlasInputs;
        buildDesc.DestAccelerationStructureData = pTlasResource->GetGPUVirtualAddress();
        buildDesc.ScratchAccelerationStructureData = pScratchResource->GetGPUVirtualAddress();

        gfx.CmdList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    }
}
