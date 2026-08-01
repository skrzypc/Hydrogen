#pragma once

#include <string>

#include "renderPass.h"
#include "frameGraphStructs.h"
#include "indexAllocators.h"
#include "descriptorHeap.h"

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
}
