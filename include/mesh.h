#pragma once

#include <vector>
#include <string>

#include <DirectXMath.h>

#include "basicTypes.h"

namespace Hydrogen
{
	struct MeshHandle
	{
		uint32 id = std::numeric_limits<uint32>::max();
		bool IsValid() const { return id != std::numeric_limits<uint32>::max(); }
		bool operator==(const MeshHandle&) const = default;
	};

	struct MeshMetadata
	{
		std::string name{};
	};

	struct Mesh
	{
		std::string name;

		std::vector<DirectX::XMFLOAT3> positions;
		std::vector<DirectX::XMFLOAT3> normals;
		std::vector<DirectX::XMFLOAT2> uvs;

		std::vector<uint32> indices;
	};
}
