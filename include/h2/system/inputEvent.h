#pragma once

#include <h2/core/basicTypes.h>

namespace Hydrogen
{
    enum class eInputEventType : uint8
    {
        KeyDown,
        KeyUp,
        MouseButtonDown,
        MouseButtonUp,
        MouseWheel,
    };

    enum class eMouseButton : uint8
    {
        Left,
        Middle,
        Right,
    };

    struct InputEvent
    {
        eInputEventType type = eInputEventType::KeyDown;
        uint8 keyCode = 0;
        eMouseButton button = eMouseButton::Left;
        float32 wheelDelta = 0.0f;

        bool IsKeyboardEvent() const
        {
            return type == eInputEventType::KeyDown || type == eInputEventType::KeyUp;
        }
        bool IsMouseEvent() const
        {
            return !IsKeyboardEvent();
        }
    };
} // namespace Hydrogen
