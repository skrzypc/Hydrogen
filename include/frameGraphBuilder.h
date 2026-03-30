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
		FGResourceHandle DefineInput(FGResourceHandle resourceHandle, FGAccess::Input inputType);
		FGResourceHandle DefineOutput(FGResourceHandle resourceHandle, FGAccess::Output writeAccess);

	private:
		FGPassNodeAccess ResolveRead(FGAccess::Input readAccess);
		FGPassNodeAccess ResolveWrite(FGAccess::Output writeAccess);

		FGResourceHandle AccessInternal(FGResourceHandle handle, FGPassNodeType direction, FGPassNodeAccess access, FGSubresourceRange range);

	private:
		FrameGraph& m_frameGraph;
		FGPass& m_pass;
	};
}