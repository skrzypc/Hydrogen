#pragma once

#include <h2/scene/entity.h>
#include <h2/core/basicTypes.h>
#include <h2/renderer/rendererUiRequest.h>

namespace Hydrogen
{
    class Scene;
    class AssetRegistry;
    class Renderer;

    struct UserInterfaceContext
    {
        Scene* pScene = nullptr;
        AssetRegistry* pAssetRegistry = nullptr;
        Renderer* pRenderer = nullptr;
        Entity* pSelection = nullptr;

        float32 deltaTime = 0.0f;
        float64 time = 0.0;

        // Requests made by panels this frame, applied by the runtime when submitting the frame.
        RendererUiRequest rendererRequest{};
    };
} // namespace Hydrogen
