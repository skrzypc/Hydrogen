
#include <string_view>

#include "verifier.h"
#include "frameGraphBuilder.h"
#include "frameGraph.h"

namespace Hydrogen
{
	FGBuilder::FGBuilder(FrameGraph& frameGraph, FGPass& pass)
		: m_frameGraph(frameGraph), m_pass(pass)
	{}

	FGResourceHandle FGBuilder::Read(FGResourceHandle handle, FGAccess::Read access)
	{
		return AccessInternal(handle, FGPassNodeType::Read, ResolveRead(access), FGSubresourceRange{});
	}

	FGResourceHandle FGBuilder::Write(FGResourceHandle handle, FGAccess::Write access)
	{
		return AccessInternal(handle, FGPassNodeType::Write, ResolveWrite(access), FGSubresourceRange{});
	}

	void FGBuilder::Read(std::string_view name, FGAccess::Read access)
	{
		Read(m_frameGraph.GetResource(name), access);
	}

	void FGBuilder::Write(std::string_view name, FGAccess::Write access)
	{
		FGResourceHandle handle = Write(m_frameGraph.GetResource(name), access);
		m_frameGraph.m_resourceRegistry[std::string(name)] = handle;
	}

	const Texture::Desc& FGBuilder::GetTextureDesc(std::string_view name) const
	{
		FGResourceHandle handle = m_frameGraph.GetResource(name);
		H2_VERIFY(handle.IsTexture(), "Resource '{}' is not a texture!", name);

		return m_frameGraph.m_textureNodes[handle.index].desc;
	}

	const Buffer::Desc& FGBuilder::GetBufferDesc(std::string_view name) const
	{
		FGResourceHandle handle = m_frameGraph.GetResource(name);
		H2_VERIFY(handle.IsBuffer(), "Resource '{}' is not a buffer!", name);

		return m_frameGraph.m_bufferNodes[handle.index].desc;
	}

	FGPassNodeAccess FGBuilder::ResolveRead(FGAccess::Read readAccess)
	{
		switch (readAccess)
		{
		case FGAccess::Read::ShaderResource:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::SRV
			};

		case FGAccess::Read::DepthStencil:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ },
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				FGUsage::DSV
			};

		case FGAccess::Read::UnorderedAccess:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS },
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				FGUsage::UAV
			};

		case FGAccess::Read::AccelerationStructure:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ, D3D12_BARRIER_LAYOUT_UNDEFINED },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::AccelerationStructure
			};

		case FGAccess::Read::CopySrc:
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

	FGPassNodeAccess FGBuilder::ResolveWrite(FGAccess::Write writeAccess)
	{
		switch (writeAccess)
		{
		case FGAccess::Write::RenderTarget:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_LAYOUT_RENDER_TARGET },
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
				FGUsage::RTV
			};

		case FGAccess::Write::DepthStencil:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_DEPTH_STENCIL, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE },
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
				FGUsage::DSV
			};

		case FGAccess::Write::UnorderedAccess:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_ALL_SHADING, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS },
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
				FGUsage::UAV
			};

		case FGAccess::Write::AccelerationStructure:
			return FGPassNodeAccess
			{
				ResourceState{ D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE, D3D12_BARRIER_LAYOUT_UNDEFINED },
				D3D12_RESOURCE_FLAG_NONE,
				FGUsage::AccelerationStructure
			};

		case FGAccess::Write::CopyDst:
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

		if (handle.IsBuffer())
		{
			FGBufferNode& node = m_frameGraph.m_bufferNodes[handle.index];

			if (direction == FGPassNodeType::Write)
			{
				H2_VERIFY(handle.version == node.version, "Stale handle usage detected");

				node.flags |= access.resourceFlags;
				node.lastWritingPassIndex = m_pass.index;

				if (node.bImported)
				{
					m_pass.bHasSideEffect = true;
				}

				handle.version++;
				node.version++;
			}
			else
			{
				H2_VERIFY(handle.version == node.version, "Stale handle usage detected");
				node.refCount++;
			}
		}
		else
		{
			FGTextureNode& node = m_frameGraph.m_textureNodes[handle.index];

			if (direction == FGPassNodeType::Write)
			{
				H2_VERIFY(handle.version == node.versions[0], "Stale handle usage detected");

				node.flags |= access.resourceFlags;
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
				node.refCount++;
			}
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
