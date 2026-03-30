
#include <d3d12.h>
#include <pix3.h>

#include "frameGraph.h"
#include "stringUtilities.h"

namespace Hydrogen
{
	void FrameGraph::Initialize(GpuDevice& device)
	{
		m_resourceCache.Initialize(device);
	}

	void FrameGraph::BeginFrame(uint64 newFrameNumber)
	{
		H2_VERIFY(m_currentFrameNumber + 1 == newFrameNumber, "Unexpected frame number!");

		m_currentFrameNumber = newFrameNumber;
	}

	const FGResourceHandle FrameGraph::CreateTexture(Texture::Desc textureDesc)
	{
		return FGResourceHandle{};
	}

	const FGResourceHandle FrameGraph::ImportTexture(Texture* pTexture, std::string_view name)
	{
		FGTextureNode node{};
		node.name = name;
		node.bImported = true;
		node.pResource = pTexture;

		node.baseResourceState = pTexture->GetState();

		node.desc = pTexture->GetDesc();

		uint32 subCount = node.desc.mipLevels * node.desc.arraySize;
		node.subresourceStates.resize(subCount, node.baseResourceState);
		node.versions.resize(subCount, 0);

		m_textureNodes.push_back(std::move(node));

		FGResourceHandle resourceHandle{};
		resourceHandle.index = static_cast<uint32>(m_textureNodes.size() - 1);
		resourceHandle.type = FGResourceType::Texture;
		resourceHandle.version = 0;

		return resourceHandle;
	}

	void FrameGraph::Compile()
	{
		// Ugly but straightforward.
		// Adjust later.
		BuildAdjacencyList();
		TopologicalSort();
		CullPasses();
		AllocateResources();
		ComputeBarriers();
		BuildDescriptors();
	}

	void FrameGraph::Execute(ID3D12GraphicsCommandList10* pCommandList)
	{
		PIXScopedEvent(pCommandList, PIX_COLOR_INDEX(0), String::Format("Frame {}", m_currentFrameNumber).c_str());

		for (auto& pass : m_passes)
		{
			if (pass.bCulled) continue;

			// Emit barriers
			D3D12_BARRIER_GROUP groups[2];
			uint32 groupCount = 0;

			if (!pass.textureBarriers.empty())
			{
				groups[groupCount++] =
				{
					.Type = D3D12_BARRIER_TYPE_TEXTURE,
					.NumBarriers = static_cast<uint32>(pass.textureBarriers.size()),
					.pTextureBarriers = pass.textureBarriers.data(),
				};
			}

			if (!pass.bufferBarriers.empty())
			{
				groups[groupCount++] =
				{
					.Type = D3D12_BARRIER_TYPE_BUFFER,
					.NumBarriers = static_cast<uint32>(pass.bufferBarriers.size()),
					.pBufferBarriers = pass.bufferBarriers.data(),
				};
			}

			if (groupCount > 0)
			{
				pCommandList->Barrier(groupCount, groups);
			}

			pass.executeFn(m_executeContext, pCommandList);
		}

		RestoreImportedResources(pCommandList);
	}

	void FrameGraph::Reset()
	{
		for (auto& node : m_textureNodes)
		{
			if (node.bImported) continue;
			if (node.pResource == nullptr) continue;

			//m_resourceCache.ReleaseTexture(node.pResource, m_currentFrame);
			node.pResource = nullptr;
		}

		for (auto& node : m_bufferNodes)
		{
			if (node.bImported) continue;
			if (node.pResource == nullptr) continue;

			//m_resourceCache.ReleaseBuffer(node.pResource, m_currentFrame);
			node.pResource = nullptr;
		}

		m_passes.clear();
		m_textureNodes.clear();
		m_bufferNodes.clear();

		m_executeContext.m_rtvMap.clear();
		m_executeContext.m_dsvMap.clear();

		//m_resourceCache.Cleanup(m_currentFrame);
	}

	void FrameGraph::BuildAdjacencyList()
	{
		// Define dependencies between all passes.
		for (uint32 passIdx = 0; passIdx < m_passes.size(); ++passIdx)
		{
			for (auto& node : m_passes[passIdx].nodes)
			{
				if (node.type != FGPassNodeType::Input) continue;

				uint32 producerIdx = node.handle.IsTexture()
					? m_textureNodes[node.handle.index].lastWritingPassIndex
					: m_bufferNodes[node.handle.index].lastWritingPassIndex;

				if (producerIdx == std::numeric_limits<uint32>::max()) continue;

				m_passes[passIdx].dependencies.push_back(producerIdx);
				m_passes[producerIdx].dependents.push_back(passIdx);
			}
		}
	}

	void FrameGraph::TopologicalSort()
	{
		uint32 passCount = static_cast<uint32>(m_passes.size());

		std::vector<uint32> passDependenciesCount(passCount, 0);
		for (uint32 i = 0; i < passCount; ++i)
		{
			passDependenciesCount[i] = static_cast<uint32>(m_passes[i].dependencies.size());
		}

		// If the pass does not have any dependencies consider it resolved.
		std::queue<uint32> resolvedPasses{};
		for (uint32 i = 0; i < passCount; ++i)
		{
			if (passDependenciesCount[i] == 0)
			{
				resolvedPasses.push(i);
			}
		}

		// Sort.
		std::vector<uint32> sortedIndices{};
		sortedIndices.reserve(passCount);
		while (!resolvedPasses.empty())
		{
			uint32 index = resolvedPasses.front();
			resolvedPasses.pop();
			sortedIndices.push_back(index);

			for (uint32 dependent : m_passes[index].dependents)
			{
				--passDependenciesCount[dependent];
				if (passDependenciesCount[dependent] == 0)
				{
					resolvedPasses.push(dependent);
				}
			}
		}

		H2_VERIFY(sortedIndices.size() == passCount, "Cycle detected in frame graph");

		// Reorder.
		std::vector<FGPass> sorted{};
		sorted.reserve(passCount);
		for (uint32 index : sortedIndices)
		{
			sorted.push_back(std::move(m_passes[index]));
		}

		m_passes = std::move(sorted);

		std::vector<uint32> unsortedToSortedRemap(passCount);
		for (uint32 i = 0; i < passCount; ++i)
		{
			unsortedToSortedRemap[sortedIndices[i]] = i;
		}

		for (auto& node : m_textureNodes)
		{
			if (node.lastWritingPassIndex != std::numeric_limits<uint32>::max())
			{
				node.lastWritingPassIndex = unsortedToSortedRemap[node.lastWritingPassIndex];
			}
		}

		for (auto& node : m_bufferNodes)
		{
			if (node.lastWritingPassIndex != std::numeric_limits<uint32>::max())
			{
				node.lastWritingPassIndex = unsortedToSortedRemap[node.lastWritingPassIndex];
			}
		}

		for (auto& pass : m_passes)
		{
			pass.index = unsortedToSortedRemap[pass.index];
		}
	}

	void FrameGraph::CullPasses()
	{
		for (auto& pass : m_passes)
		{
			pass.refCount = 0;
			for (auto& node : pass.nodes)
			{
				if (node.type != FGPassNodeType::Output)
				{
					continue;
				}

				pass.refCount += node.handle.IsTexture()
					? m_textureNodes[node.handle.index].refCount
					: m_bufferNodes[node.handle.index].refCount;
			}
		}

		// Backward flood-fill
		for (int32 i = static_cast<int32>(m_passes.size()) - 1; i >= 0; --i)
		{
			FGPass& pass = m_passes[i];

			if (pass.bHasSideEffect || pass.refCount > 0)
			{
				continue;
			}

			pass.bCulled = true;

			for (uint32 index : pass.dependencies)
			{
				m_passes[index].refCount--;
			}
		}
	}

	void FrameGraph::AllocateResources()
	{
		for (auto& node : m_textureNodes)
		{
			if (node.bImported || node.refCount) continue;

			//node.pResource = m_resourceCache.AcquireTexture(node.desc, node.flags, m_currentFrame);

			H2_VERIFY(node.pResource != nullptr, "Failed to allocate texture resource for node: {}", node.name);

			node.pResource->SetName(String::ToWide(node.name));
		}

		for (auto& node : m_bufferNodes)
		{
			if (node.bImported || node.refCount) continue;

			//node.pResource = m_resourceCache.AcquireBuffer(node.desc, node.flags, m_currentFrame);

			H2_VERIFY(node.pResource != nullptr, "Failed to allocate buffer resource for node: {}", node.name);

			node.pResource->SetName(String::ToWide(node.name));
		}
	}

	void FrameGraph::ComputeBarriers()
	{
		for (auto& pass : m_passes)
		{
			if (pass.bCulled) continue;

			for (auto& passNode : pass.nodes)
			{
				bool isWrite = passNode.type == FGPassNodeType::Output;

				if (passNode.handle.IsTexture())
				{
					FGTextureNode& node = m_textureNodes[passNode.handle.index];

					uint32 mipStart = passNode.range.mipOffset;
					uint32 mipEnd = (passNode.range.mipLevelsCount == std::numeric_limits<uint32>::max())
						? node.desc.mipLevels
						: mipStart + passNode.range.mipLevelsCount;
					uint32 arrayStart = passNode.range.arrayOffset;
					uint32 arrayEnd = (passNode.range.arraySlicesCount == std::numeric_limits<uint32>::max())
						? node.desc.arraySize
						: arrayStart + passNode.range.arraySlicesCount;

					for (uint32 array = arrayStart; array < arrayEnd; ++array)
					{
						for (uint32 mip = mipStart; mip < mipEnd; ++mip)
						{
							uint32           idx = mip + array * node.desc.mipLevels;
							ResourceState& cur = node.subresourceStates[idx];
							const ResourceState& required = passNode.access.resourceState;

							bool layoutChange = cur.layout != required.layout;
							bool accessChange = cur.access != required.access;
							bool syncChange = cur.sync != required.sync;

							if (!layoutChange && !accessChange && !syncChange) continue;

							bool isFirstAccess = cur.access == D3D12_BARRIER_ACCESS_NO_ACCESS;

							pass.textureBarriers.push_back(D3D12_TEXTURE_BARRIER
								{
									.SyncBefore = cur.sync,
									.SyncAfter = required.sync,
									.AccessBefore = cur.access,
									.AccessAfter = required.access,
									.LayoutBefore = cur.layout,
									.LayoutAfter = required.layout,
									.pResource = node.pResource->GetResource(),
									.Subresources =
								{
									.IndexOrFirstMipLevel = mip,
									.NumMipLevels = 1,
									.FirstArraySlice = array,
									.NumArraySlices = 1,
									.FirstPlane = 0,
									.NumPlanes = 1,
								},
								.Flags = (isFirstAccess && isWrite)
								? D3D12_TEXTURE_BARRIER_FLAG_DISCARD
								: D3D12_TEXTURE_BARRIER_FLAG_NONE,
								});

							// Update tracked state
							cur = required;
						}
					}
				}
				else
				{
					FGBufferNode& node = m_bufferNodes[passNode.handle.index];
					ResourceState& cur = node.resourceState;
					const ResourceState& required = passNode.access.resourceState;

					bool accessChange = cur.access != required.access;
					bool syncChange = cur.sync != required.sync;

					if (!accessChange && !syncChange) continue;

					pass.bufferBarriers.push_back(D3D12_BUFFER_BARRIER
						{
							.SyncBefore = cur.sync,
							.SyncAfter = required.sync,
							.AccessBefore = cur.access,
							.AccessAfter = required.access,
							.pResource = node.pResource->GetResource(),
							.Offset = 0,
							.Size = UINT64_MAX,
						});

					cur = required;
				}
			}
		}
	}

	void FrameGraph::BuildDescriptors()
	{
		for (auto& pass : m_passes)
		{
			if (pass.bCulled) continue;

			for (auto& passNode : pass.nodes)
			{
				if (!passNode.handle.IsTexture()) continue;

				FGTextureNode& node = m_textureNodes[passNode.handle.index];

				switch (passNode.access.resourceUsage)
				{
				case FGUsage::RTV:
					// Does it make sense to copy this to the execute context?
					m_executeContext.m_rtvMap[passNode.handle.index] = m_resourceCache.GetRTV(node.pResource, passNode.range).dxCpuHandle;
					break;

				case FGUsage::DSV:
					// Does it make sense to copy this to the execute context?
					m_executeContext.m_dsvMap[passNode.handle.index] = m_resourceCache.GetDSV(node.pResource, passNode.range).dxCpuHandle;
					break;

				case FGUsage::SRV:
				case FGUsage::UAV:
					// TODO: bindless
					break;

				case FGUsage::None:
					break;
				}
			}
		}
	}

	void FrameGraph::RestoreImportedResources(ID3D12GraphicsCommandList7* cmd)
	{
		std::vector<D3D12_TEXTURE_BARRIER> restoreBarriers;

		for (auto& node : m_textureNodes)
		{
			if (!node.bImported) continue;

			for (uint32 i = 0; i < node.subresourceStates.size(); ++i)
			{
				ResourceState& cur = node.subresourceStates[i];

				if (cur.layout == node.baseResourceState.layout) continue;

				restoreBarriers.push_back(D3D12_TEXTURE_BARRIER
					{
						.SyncBefore = cur.sync,
						.SyncAfter = D3D12_BARRIER_SYNC_NONE,
						.AccessBefore = cur.access,
						.AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS,
						.LayoutBefore = cur.layout,
						.LayoutAfter = node.baseResourceState.layout,
						.pResource = node.pResource->GetResource(),
						.Subresources =
					{
						.IndexOrFirstMipLevel = i % node.desc.mipLevels,
						.NumMipLevels = 1,
						.FirstArraySlice = i / node.desc.mipLevels,
						.NumArraySlices = 1,
						.FirstPlane = 0,
						.NumPlanes = 1,
					},
					.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE,
					});
			}
		}

		if (restoreBarriers.empty()) return;

		D3D12_BARRIER_GROUP group =
		{
			.Type = D3D12_BARRIER_TYPE_TEXTURE,
			.NumBarriers = static_cast<uint32>(restoreBarriers.size()),
			.pTextureBarriers = restoreBarriers.data(),
		};

		cmd->Barrier(1, &group);
	}
}