#pragma once

#include <vector>

#include <DirectXMath.h>

#include "basicTypes.h"
#include "hydrogenMath.h"
#include "mesh.h"
#include "light.h"

namespace Hydrogen
{
    struct RenderObject
    {
        MeshHandle mesh{};
        DirectX::XMFLOAT4X4 worldMatrix{};
    };

    struct RenderLight
    {
        Light light{};
        Vector3 position{};
        Vector3 direction{};
    };

    struct CameraData
    {
        Vector3 position{};
        Quaternion rotation{};
        float32 fovYDeg = 45.0f;
        float32 nearZ = 0.01f;
        float32 farZ = 100.0f;
        float32 exposure = 2.0f;
    };

    struct RenderScene
    {
        std::vector<RenderObject> objects{};
        std::vector<RenderLight> lights{};
        CameraData camera{};
    };
}
