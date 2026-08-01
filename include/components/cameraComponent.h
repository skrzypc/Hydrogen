#pragma once

#include "basicTypes.h"

namespace Hydrogen
{
    struct CameraComponent
    {
        float32 fovYDeg = 45.0f;
        float32 nearZ = 0.01f;
        float32 farZ = 100.0f;
        float32 exposure = 2.0f;
    };
}
