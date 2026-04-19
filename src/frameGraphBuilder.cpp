
#include <string_view>

#include "verifier.h"
#include "frameGraphBuilder.h"
#include "frameGraph.h"

namespace Hydrogen
{
	FGBuilder::FGBuilder(FrameGraph& frameGraph, FGPass& pass)
		: m_frameGraph(frameGraph), m_pass(pass)
	{}

	FGResourceHandle FGBuilder::Read(FGResourceHandle handle, FGAccess::Input access)
	{
		return AccessInternal(handle, FGPassNodeType::Input, ResolveRead(access), FGSubresourceRange{});
	}

	FGResourceHandle FGBuilder::Write(FGResourceHandle handle, FGAccess::Output access)
	{
		return AccessInternal(handle, FGPassNodeType::Output, ResolveWrite(access), FGSubresourceRange{});
	}

	FGResourceHandle FGBuilder::Read(std::string_view name, FGAccess::Input access)
	{
		return Read(m_frameGraph.GetResource(name), access);
	}

	FGResourceHandle FGBuilder::Write(std::string_view name, FGAccess::Output access)
	{
		FGResourceHandle handle = Write(m_frameGraph.GetResource(name), access);
		m_frameGraph.m_resourceRegistry[String::ToUpper(name)] = handle;
		return handle;
	}

	const Texture::Desc& FGBuilder::GetTextureDesc(std::string_view name) const
	{
		FGResourceHandle handle = m_frameGraph.GetResource(name);
		return m_frameGraph.m_textureNodes[handle.index].desc;
	}

	FGPassNodeAccess FGBuilder::ResolveRead(FGAccess::Input readAccess)
	{
		switch (readAccess)
		{
		case FGAccess::Input::ShaderResource:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::SRV
			};

		case FGAccess::Input::DepthStencil:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ },
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				FGUsage::DSV
			};

		case FGAccess::Input::UnorderedAccess:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS },
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				FGUsage::UAV
			};

		case FGAccess::Input::CopySrc:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_SOURCE, D3D12_BARRIER_LAYOUT_COPY_SOURCE },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::None
			};
		default:
			std::unreachable();
			break;
		}
	}

	FGPassNodeAccess FGBuilder::ResolveWrite(FGAccess::Output writeAccess)
	{
		switch (writeAccess)
		{
		case FGAccess::Output::RenderTarget:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_LAYOUT_RENDER_TARGET },
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				FGUsage::RTV
			};

		case FGAccess::Output::DepthStencil:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE },
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				FGUsage::DSV
			};

		case FGAccess::Output::UnorderedAccess:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS },
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				FGUsage::UAV
			};

		case FGAccess::Output::CopyDst:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_LAYOUT_COPY_DEST },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::None
			};
		default:
			std::unreachable();
			break;
		}
	}

	FGResourceHandle FGBuilder::AccessInternal(FGResourceHandle handle, FGPassNodeType direction, FGPassNodeAccess access, FGSubresourceRange range)
	{
		H2_VERIFY(handle.IsValid(), "Accessing invalid handle!");

		FGTextureNode& node = m_frameGraph.m_textureNodes[handle.index];

		if (direction == FGPassNodeType::Output)
		{
			H2_VERIFY(handle.version == node.versions[0], "Stale handle usage detected");

			node.flags |= access.resourceFlags;

			//uint32 mipStart = range.mipOffset;
			//uint32 mipEnd = (range.mipLevelsCount == 0xffffffff) ? node.desc.mipLevels : mipStart + range.mipLevelsCount;
			//uint32 arrayStart = range.arrayOffset;
			//uint32 arrayEnd = (range.arraySlicesCount == 0xffffffff) ? node.desc.arraySize : arrayStart + range.arraySlicesCount;

			//for (uint32 array = arrayStart; array < arrayEnd; ++array)
			//{
			//	for (uint32 mip = mipStart; mip < mipEnd; ++mip)
			//	{
			//		node.versions[mip + array * node.desc.mipLevels]++;
			//	}
			//}

			node.lastWritingPassIndex = m_pass.index;

			if (node.bImported)
			{
				m_pass.bHasSideEffect = true;
			}

			handle.version++;
			node.versions[0]++;
		}
		else
		{
			H2_VERIFY(handle.version == node.versions[0], "Stale handle usage detected");

			// Increment ref count — keeps this node and its producer alive during culling
			node.refCount++;
		}

		m_pass.nodes.push_back(
			FGPassNode
			{
				.handle = handle,
				.type = direction,
				.access = access,
				.range = range,
			}
		);

		return handle;
	}
}
