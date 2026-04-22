#pragma once

#include <vector>
#include <string_view>

#include "staticMesh.h"

namespace Hydrogen
{
	class MeshLoader
	{
	public:
		// Returns one StaticMesh per mesh primitive in the file.
		static std::vector<StaticMesh> Load(std::string_view path);
	};
}
