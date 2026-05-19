#pragma once

#include <vector>

#include <DirectXMath.h>

#include "basicTypes.h"
#include "hydrogenMath.h"
#include "mesh.h"

namespace Hydrogen
{
    struct RenderObject
    {
        MeshHandle mesh{};
        DirectX::XMFLOAT4X4 worldMatrix{};
    };

    struct CameraData
    {
        Vector3 position{};
        Quaternion rotation{};
        float32 fovYDeg = 45.0f;
        float32 nearZ = 0.01f;
        float32 farZ = 100.0f;
    };

    struct RenderScene
    {
        std::vector<RenderObject> objects{};
        CameraData camera{};
    };
}
