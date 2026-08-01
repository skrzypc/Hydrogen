#include "renderPasses/imguiPass.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>

#include "device.h"
#include "graphicsContext.h"
#include "frameGraphBuilder.h"

namespace Hydrogen
{
	void ImguiPass::Initialize(GpuDevice& device, ShaderCompiler&)
	{
		IMGUI_CHECKVERSION();
		ImGui::StyleColorsDark();

		m_descriptorBundle.descHeap = &device.GetDescriptorHeap<eDescriptorHeapType::CBV_SRV_UAV>();
		m_descriptorBundle.descAllocator = device.RequestDescriptorAllocator<FreeListIndexAllocator>(8, eDescriptorHeapType::CBV_SRV_UAV);

		ImGui_ImplDX12_InitInfo initInfo{};
		initInfo.Device = device.GetDxDevice();
		initInfo.CommandQueue = device.GetDxQueue<eQueueType::Direct>();
		initInfo.NumFramesInFlight = Config::FramesInFlight;
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		initInfo.SrvDescriptorHeap = m_descriptorBundle.descHeap->GetDxHeap();
		initInfo.UserData = &m_descriptorBundle;

		initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu)
		{
			auto* alloc = static_cast<HeapSlotAllocator*>(info->UserData);
			uint32 index = alloc->descAllocator.Allocate();
			*cpu = alloc->descHeap->GetCpuHandle(index);
			*gpu = alloc->descHeap->GetGpuHandle(index);
		};

		initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE)
		{
			auto* alloc = static_cast<HeapSlotAllocator*>(info->UserData);
			uint32 index = static_cast<uint32>((cpu.ptr - alloc->descHeap->GetCpuHandle(0).ptr) / alloc->descHeap->GetDescriptorSize());
			alloc->descAllocator.Free(index);
		};

		ImGui_ImplDX12_Init(&initInfo);
	}

	void ImguiPass::Setup(FGBuilder& builder)
	{
		builder.Write(target, FGAccess::Write::RenderTarget);
	}

	void ImguiPass::Execute(FGExecuteContext& fgExecuteContext, GraphicsContext& graphicsContext)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv = fgExecuteContext.GetRTV(target);
		graphicsContext.CmdList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

		ImGui_ImplDX12_RenderDrawData(pDrawData, graphicsContext.CmdList());
	}

	void ImguiPass::Shutdown()
	{
		ImGui_ImplDX12_Shutdown();
	}
}
