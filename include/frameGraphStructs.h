#pragma once

#include <string>
#include <string_view>
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

	// FG Read/Write.
	enum class FGPassNodeType : uint8
	{
		Read = 0,
		Write = 1,
		Unknown
	};

	enum class FGUsage : uint8
	{
		None = 0,   // CopySrc / CopyDst
		RTV,
		DSV,
		SRV,
		UAV,
		AccelerationStructure,
	};

	namespace FGAccess
	{
		enum class Read : uint8
		{
			ShaderResource,
			DepthStencil,
			UnorderedAccess,
			AccelerationStructure,
			CopySrc,
		};

		enum class Write : uint8
		{
			RenderTarget,
			DepthStencil,
			UnorderedAccess,
			AccelerationStructure,
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

		uint8 version = 0u; // Buffers have no subresources, so a single counter mirrors FGTextureNode::versions.

		D3D12_RESOURCE_FLAGS flags{};
		ResourceState resourceState{};

		uint32 refCount = 0;
		uint32 lastWritingPassIndex = std::numeric_limits<uint32>::max();

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
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(std::string_view name) const
		{
			const uint32 key = KeyFromName(name);
			H2_VERIFY_FATAL(m_rtvMap.contains(key), "No RTV found for resource '{}'!", name);

			return m_rtvMap.at(key);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(std::string_view name) const
		{
			const uint32 key = KeyFromName(name);
			H2_VERIFY_FATAL(m_dsvMap.contains(key), "No DSV found for resource '{}'!", name);

			return m_dsvMap.at(key);
		}

		ID3D12Resource* GetResource(std::string_view name) const
		{
			const uint32 key = KeyFromName(name);
			H2_VERIFY_FATAL(m_resourceMap.contains(key), "No resource found for resource '{}'!", name);

			return m_resourceMap.at(key);
		}

		uint32 GetSRVIndex(std::string_view name) const
		{
			const uint32 key = KeyFromName(name);
			H2_VERIFY_FATAL(m_srvIndexMap.contains(key), "No SRV found for resource '{}'!", name);

			return m_srvIndexMap.at(key);
		}

		uint32 GetUAVIndex(std::string_view name) const
		{
			const uint32 key = KeyFromName(name);
			H2_VERIFY_FATAL(m_uavIndexMap.contains(key), "No UAV found for resource '{}'!", name);

			return m_uavIndexMap.at(key);
		}

	private:
		friend class FrameGraph;

		// Texture and buffer handles have separate index spaces, so combine type and index into a unique key.
		static uint32 ResourceKey(FGResourceHandle handle)
		{
			return (static_cast<uint32>(handle.type) << 16) | handle.index;
		}

		uint32 KeyFromName(std::string_view name) const
		{
			const std::string resourceName(name);
			H2_VERIFY_FATAL(m_nameToKey.contains(resourceName), "Resource '{}' was not accessed by any live pass!", name);

			return m_nameToKey.at(resourceName);
		}

		std::unordered_map<uint32, D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvMap;
		std::unordered_map<uint32, D3D12_CPU_DESCRIPTOR_HANDLE> m_dsvMap;
		std::unordered_map<uint32, uint32> m_srvIndexMap;
		std::unordered_map<uint32, uint32> m_uavIndexMap;
		std::unordered_map<uint32, ID3D12Resource*> m_resourceMap;
		std::unordered_map<std::string, uint32> m_nameToKey;
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