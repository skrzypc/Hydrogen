#pragma once

#include <string_view>

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
		FGResourceHandle Read(FGResourceHandle handle, FGAccess::Read access);
		FGResourceHandle Write(FGResourceHandle handle, FGAccess::Write access);

		FGResourceHandle Read(std::string_view name, FGAccess::Read access);
		FGResourceHandle Write(std::string_view name, FGAccess::Write access);

		const Texture::Desc& GetTextureDesc(std::string_view name) const;

	private:
		FGPassNodeAccess ResolveRead(FGAccess::Read access);
		FGPassNodeAccess ResolveWrite(FGAccess::Write access);

		FGResourceHandle AccessInternal(FGResourceHandle handle, FGPassNodeType direction, FGPassNodeAccess access, FGSubresourceRange range);

	private:
		FrameGraph& m_frameGraph;
		FGPass& m_pass;
	};
}