#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include <d3d12.h>

#include "texture.h"
#include "buffer.h"
#include "basicTypes.h"
#include "verifier.h"

namespace Hydrogen
{
	class GraphicsContext;

	// Texture/Buffer
	enum class FGResourceType : uint8
	{
		Texture = 0,
		Buffer = 1,
		Undefined = 255,
	};

	// FG Input/Output.
	enum class FGPassNodeType : uint8
	{
		Input = 0,
		Output = 1,
		Unknown
	};

	enum class FGUsage : uint8
	{
		None = 0,   // CopySrc / CopyDst
		RTV,
		DSV,
		SRV,
		UAV,
	};

	namespace FGAccess
	{
		enum class Input : uint8
		{
			ShaderResource,
			DepthStencil,
			UnorderedAccess,
			CopySrc,
		};

		enum class Output : uint8
		{
			RenderTarget,
			DepthStencil,
			UnorderedAccess,
			CopyDst,
		};
	}

	struct FGResourceHandle
	{
		uint16 index = std::numeric_limits<uint16>::max();
		FGResourceType type = FGResourceType::Undefined;
		uint8 version = 0u;

		bool IsValid() const { return index != std::numeric_limits<uint16>::max(); }
		bool IsTexture() const { return type == FGResourceType::Texture; }
		bool IsBuffer() const { return type == FGResourceType::Buffer; }

		FGResourceType GetType() const { return type; }
		bool operator==(const FGResourceHandle&) const = default;
	};

	// Matches with enhanced barriers granularity.
	struct FGSubresourceRange
	{
		static constexpr uint32 All = std::numeric_limits<uint32>::max();

		uint32 mipOffset = 0;
		uint32 mipLevelsCount = All;
		uint32 arrayOffset = 0;
		uint32 arraySlicesCount = All;
	};

	struct FGTextureNode
	{
		std::string name = "Unknown";
		Texture::Desc desc{};

		Texture* pResource = nullptr;

		std::vector<uint8> versions{}; // per subresource.
		std::vector<ResourceState> subresourceStates{}; // per subresource.

		D3D12_RESOURCE_FLAGS flags{};
		ResourceState baseResourceState{};

		uint32 refCount = 0; // number of passes reading this node.
		uint32 lastWritingPassIndex = std::numeric_limits<uint32>::max(); // index of last pass that wrote this node.

		bool bImported = false;

		uint32 GetSubresourceCount() const
		{
			return desc.mipLevels * desc.arraySize;
		}

		uint32 GetSubresourceIndex(uint32 mip, uint32 arraySlice) const
		{
			return mip + arraySlice * desc.mipLevels;
		}
	};

	struct FGBufferNode
	{
		std::string name = "Unknown";
		Buffer::Desc desc{};

		Buffer* pResource = nullptr;

		D3D12_RESOURCE_FLAGS flags{};
		ResourceState resourceState{};

		uint32 refCount = 0;
		uint32 lastWritingPassIndex = -1;

		bool bImported = false;
	};

	struct FGPassNodeAccess
	{
		ResourceState resourceState;
		D3D12_RESOURCE_FLAGS resourceFlags;
		FGUsage resourceUsage;
	};

	struct FGPassNode
	{
		FGResourceHandle handle{};

		FGPassNodeType type = FGPassNodeType::Unknown;
		FGPassNodeAccess access{};

		FGSubresourceRange range{};

		//D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor{}; // Assigned at compile stage.
	};

	class FGExecuteContext
	{
	public:
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(FGResourceHandle handle) const
		{
			H2_VERIFY_FATAL(handle.IsTexture() && m_rtvMap.contains(handle.index), "No RTV found for handle!");

			return m_rtvMap.at(handle.index);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(FGResourceHandle handle) const
		{
			H2_VERIFY_FATAL(handle.IsTexture() && m_dsvMap.contains(handle.index), "No DSV found for handle!");

			return m_dsvMap.at(handle.index);
		}

	private:
		friend class FrameGraph;

		std::unordered_map<uint32, D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvMap;
		std::unordered_map<uint32, D3D12_CPU_DESCRIPTOR_HANDLE> m_dsvMap;
	};

	struct FGPass
	{
		std::vector<FGPassNode> nodes{};

		std::vector<uint32> dependencies{}; // Indices of the passes that given pass must wait for.
		std::vector<uint32> dependents{}; // Indices of the passes that must wait for the given pass.

		std::vector<D3D12_TEXTURE_BARRIER> textureBarriers{};
		std::vector<D3D12_BUFFER_BARRIER> bufferBarriers{};

		std::move_only_function<void(FGExecuteContext&, GraphicsContext&)> executeFn{};

		std::string name = "Unknown";

		uint32 index = std::numeric_limits<uint32>::max();
		uint32 refCount = 0;

		bool bCulled = false;
		bool bHasSideEffect = false;
	};
}