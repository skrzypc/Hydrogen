#pragma once

#include <optional>

#include <h2/renderer/renderPipelines/renderPipeline.h>

namespace Hydrogen
{
    // What the user asked the renderer to change through its UI. Applied by Renderer::SubmitFrame.
    struct RendererUiRequest
    {
        std::optional<eRenderPipelineType> renderPipelineType{};
    };
} // namespace Hydrogen
