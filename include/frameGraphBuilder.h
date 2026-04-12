#pragma once

#include <string_view>

#include "frameResources.h"
#include "frameGraphStructs.h"

namespace Hydrogen
{
	class FrameGraph;

	class FGBuilder
	{
		friend class FrameGraph;
	private:
		FGBuilder(FrameGraph& frameGraph, FGPass& pass);
		~FGBuilder() = default;
		FGBuilder(const FGBuilder&) = delete;
		FGBuilder& operator=(const FGBuilder&) = delete;
		FGBuilder(FGBuilder&&) noexcept = default;
		FGBuilder& operator=(FGBuilder&&) noexcept = default;

	public:
		FGResourceHandle Read(FGResourceHandle handle, FGAccess::Input access);
		FGResourceHandle Write(FGResourceHandle handle, FGAccess::Output access);

		FGResourceHandle Read(eFrameResource resource, FGAccess::Input access);
		FGResourceHandle Write(eFrameResource resource, FGAccess::Output access);

		const Texture::Desc& GetTextureDesc(eFrameResource resource) const;

	private:
		FGPassNodeAccess ResolveRead(FGAccess::Input access);
		FGPassNodeAccess ResolveWrite(FGAccess::Output access);

		FGResourceHandle AccessInternal(FGResourceHandle handle, FGPassNodeType direction, FGPassNodeAccess access, FGSubresourceRange range);

	private:
		FrameGraph& m_frameGraph;
		FGPass& m_pass;
	};
}