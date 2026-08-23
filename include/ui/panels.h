#pragma once

#include <DirectXMath.h>

#include "entity.h"
#include "ui/panel.h"

namespace Hydrogen
{
    struct Transform;
    struct Light;
    struct CameraComponent;

    class ScenePanel : public IPanel
    {
    public:
        void Draw(UiContext& context) override;
        const char* GetName() const override { return "Scene"; }
    };

    class InspectorPanel : public IPanel
    {
    public:
        void Draw(UiContext& context) override;
        const char* GetName() const override { return "Inspector"; }

    private:
        void DrawTransformEditor(Transform& transform);
        void DrawLightEditor(Light& light);
        void DrawCameraEditor(CameraComponent& camera);

        Entity m_lastSelection{};
        DirectX::XMFLOAT4 m_lastKnownRotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 m_cachedEulerDeg{};
    };

    class StatsPanel : public IPanel
    {
    public:
        void Draw(UiContext& context) override;
        const char* GetName() const override { return "Stats"; }
    };

    class RendererPanel : public IPanel
    {
    public:
        void Draw(UiContext& context) override;
        const char* GetName() const override { return "Renderer"; }
    };
}
