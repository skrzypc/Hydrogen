#pragma once

#include <DirectXMath.h>

#include <h2/scene/entity.h>
#include <h2/ui/userInterfaceContext.h>

namespace Hydrogen
{
    struct Transform;
    struct Light;
    struct CameraComponent;
    struct Material;

    class IPanel
    {
    public:
        virtual ~IPanel() = default;

        virtual void Draw(UserInterfaceContext& context) = 0;
        virtual const char* GetName() const = 0;

        bool IsOpen() const
        {
            return m_open;
        }
        void SetOpen(bool open)
        {
            m_open = open;
        }

    protected:
        bool m_open = true;
    };

    class ScenePanel : public IPanel
    {
    public:
        void Draw(UserInterfaceContext& context) override;
        const char* GetName() const override
        {
            return "Scene";
        }
    };

    class InspectorPanel : public IPanel
    {
    public:
        void Draw(UserInterfaceContext& context) override;
        const char* GetName() const override
        {
            return "Inspector";
        }

    private:
        void DrawTransformEditor(Transform& transform);
        void DrawLightEditor(Light& light);
        void DrawCameraEditor(CameraComponent& camera);
        bool DrawMaterialEditor(Material& material);

        Entity m_lastSelection{};
        DirectX::XMFLOAT4 m_lastKnownRotation{0.0f, 0.0f, 0.0f, 1.0f};
        DirectX::XMFLOAT3 m_cachedEulerDeg{};
    };

    class StatsPanel : public IPanel
    {
    public:
        void Draw(UserInterfaceContext& context) override;
        const char* GetName() const override
        {
            return "Stats";
        }
    };

    class RendererPanel : public IPanel
    {
    public:
        void Draw(UserInterfaceContext& context) override;
        const char* GetName() const override
        {
            return "Renderer";
        }
    };
} // namespace Hydrogen
