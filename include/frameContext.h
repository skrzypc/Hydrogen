#pragma once

#include <dxgiformat.h>

#include "basicTypes.h"
#include "renderScene.h"

namespace Hydrogen
{
    class GpuScene;

    struct FrameContext
    {
        const GpuScene& gpuScene;
        const RenderScene& renderScene;

        uint32 renderWidth = 0u;
        uint32 renderHeight = 0u;
        uint32 displayWidth = 0u;
        uint32 displayHeight = 0u;
        DXGI_FORMAT displayFormat = DXGI_FORMAT_UNKNOWN;

        uint64 frameNumber = 0;
        uint32 frameIndex = 0;

        float64 time = 0.0;
        float32 deltaTime = 0.0f;

		bool sceneChanged = false;
    };
}
