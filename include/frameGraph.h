#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <array>

#include <d3d12.h>

#include "texture.h"
#include "buffer.h"
#include "basicTypes.h"
#include "frameGraphBuilder.h"
#include "frameGraphResourceCache.h"
#include "stringUtilities.h"
#include "renderPasses/renderPass.h"
#include "graphicsContext.h"

namespace Hydrogen
{
	class FrameGraph
	{
		friend class FGBuilder;
	public:
		FrameGraph() = default;
		~FrameGraph() = default;
		FrameGraph(const FrameGraph&) = delete;
		FrameGraph& operator=(const FrameGraph&) = delete;
		FrameGraph(FrameGraph&&) noexcept = default;
		FrameGraph& operator=(FrameGraph&&) noexcept = default;

		void Initialize(GpuDevice& device);

		void BeginFrame(uint64 newFrameNumber);

		const FGResourceHandle CreateTexture(Texture::Desc textureDesc);
		const FGResourceHandle ImportTexture(Texture* pTexture, std::string_view name);
		void Import(eFrameResource name, Texture* pTexture);

		FGResourceHandle GetResource(eFrameResource name) const;

		void AddPass(std::string_view passName, IRenderPass& pass);

		template<typename PassDataT, typename SetupFn, typename ExecuteFn>
		void AddPass(
			std::string_view passName,
			SetupFn&& setupFn,
			ExecuteFn&& executeFn
		)
		{
			FGPass& pass = m_passes.emplace_back();
			pass.name = passName;
			pass.index = static_cast<uint32>(m_passes.size() - 1u);

			FGBuilder builder(*this, pass);
			auto pPassData = std::make_unique<PassDataT>();

			setupFn(builder, *pPassData.get());

			pass.executeFn = [pData = std::move(pPassData), fn = std::forward<decltype(executeFn)>(executeFn)]
			(FGExecuteContext& ctx, GraphicsContext& gfx) mutable
				{
					fn(*pData, ctx, gfx);
				};
		}

		void Compile();
		void Execute(GraphicsContext& gfx);

		void Reset();

	private:
		void BuildAdjacencyList();
		void TopologicalSort();
		void CullPasses();
		void AllocateResources();
		void ComputeBarriers();
		void BuildDescriptors();

		void RestoreImportedResources(ID3D12GraphicsCommandList7* cmd);

	private:
		uint64 m_currentFrameNumber = std::numeric_limits<uint64>::max();

		FGExecuteContext m_executeContext{};
		FGResourceCache m_resourceCache{};

		std::vector<FGPass> m_passes{};

		std::vector<FGTextureNode> m_textureNodes{};
		std::vector<FGBufferNode> m_bufferNodes{};

		std::array<FGResourceHandle, static_cast<uint32>(eFrameResource::Count)> m_resourceRegistry{};
	};
}