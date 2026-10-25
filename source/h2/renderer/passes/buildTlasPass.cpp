#include <d3d12.h>

#include <h2/renderer/gpu/device.h>
#include <h2/renderer/gpu/scene/gpuScene.h>
#include <h2/renderer/gpu/commands/graphicsContext.h>
#include <h2/renderer/framegraph/frameGraphBuilder.h>
#include <h2/renderer/passes/buildTlasPass.h>

namespace Hydrogen
{
    void BuildTlasPass::Initialize(GpuDevice& device, ShaderCompiler& shaderCompiler) {}

    void BuildTlasPass::Setup(FGBuilder& builder)
    {
        builder.Write("TLAS", FGAccess::Write::AccelerationStructure);
        builder.Write("TLASScratch", FGAccess::Write::UnorderedAccess);
    }

    void BuildTlasPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
    {
        ID3D12Resource* pTlasResource = fgExecuteContext.GetResource("TLAS");
        ID3D12Resource* pScratchResource = fgExecuteContext.GetResource("TLASScratch");

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

        graphicsContext.CmdList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    }
} // namespace Hydrogen
