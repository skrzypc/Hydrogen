#pragma once

#include <functional>

#include "entity.h"
#include "basicTypes.h"

namespace Hydrogen
{
    class Scene;
    class AssetRegistry;

    struct UiContext
    {
        Scene* pScene = nullptr;
        AssetRegistry* pAssetRegistry = nullptr;
        std::function<void()> fnBuildRendererUi;

        Entity selection{};

        float32 deltaTime = 0.0f;
        float64 time = 0.0;
    };
}
