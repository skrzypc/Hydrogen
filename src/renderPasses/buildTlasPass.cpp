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
        m_pDevice = &device;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.RaytracingAccelerationStructure.Location = 0;
        m_tlasSrv = device.CreateShaderResourceView(static_cast<const Buffer*>(nullptr), srvDesc);
    }

    BuildTlasPass::TlasSizes BuildTlasPass::QuerySizes(uint32 instanceCount)
    {
        if (instanceCount == m_lastQueriedInstanceCount)
        {
            return m_cachedSizes;
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs{};
        tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        tlasInputs.NumDescs = instanceCount;
        tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
        m_pDevice->GetDxDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuild);

        m_cachedSizes = TlasSizes{ .resultSize = prebuild.ResultDataMaxSizeInBytes, .scratchSize = prebuild.ScratchDataSizeInBytes };
        m_lastQueriedInstanceCount = instanceCount;

        return m_cachedSizes;
    }

    void BuildTlasPass::Setup(FGBuilder& builder)
    {
        builder.Write(m_tlasHandle, FGAccess::Write::AccelerationStructure);
        builder.Write(m_scratchHandle, FGAccess::Write::UnorderedAccess);
    }

    void BuildTlasPass::Execute(FGExecuteContext& ctx, GraphicsContext& gfx)
    {
        ID3D12Resource* pTlasResource = ctx.GetResource(m_tlasHandle);
        ID3D12Resource* pScratchResource = ctx.GetResource(m_scratchHandle);

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

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.RaytracingAccelerationStructure.Location = pTlasResource->GetGPUVirtualAddress();
        m_pDevice->UpdateShaderResourceView(m_tlasSrv, srvDesc);
    }
}
