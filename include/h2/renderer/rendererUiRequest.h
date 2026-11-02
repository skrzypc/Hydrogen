#pragma once

#include <optional>

#include <h2/renderer/renderers/renderer.h>

namespace Hydrogen
{
    // What the user asked the renderer to change through its UI. Applied by Renderer::SubmitFrame.
    struct RendererUiRequest
    {
        std::optional<eRendererType> rendererType{};
    };
} // namespace Hydrogen
