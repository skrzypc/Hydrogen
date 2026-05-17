#pragma once

#include <vector>

#include <DirectXMath.h>

#include "mesh.h"

namespace Hydrogen
{
    struct RenderObject
    {
        MeshHandle mesh{};
        DirectX::XMFLOAT4X4 worldMatrix{};
    };

    struct RenderScene
    {
        std::vector<RenderObject> objects{};
    };
}
