#pragma once

#include <string>

#include "basicTypes.h"

namespace Hydrogen
{
	struct GpuMesh
	{
		std::string name;

		uint32 baseVertex = 0;
		uint32 vertexCount = 0;

		uint32 baseIndex = 0;
		uint32 indexCount = 0;
	};
}
