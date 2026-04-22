#pragma once

#include <vector>
#include <string>

#include <DirectXMath.h>

#include "basicTypes.h"

namespace Hydrogen
{
	struct StaticMesh
	{
		std::string name;

		std::vector<DirectX::XMFLOAT3> positions;
		std::vector<DirectX::XMFLOAT3> normals;
		std::vector<DirectX::XMFLOAT2> uvs;

		std::vector<uint32> indices;
	};
}
