#pragma once

#include <h2/core/basicTypes.h>

#include <h2/hydrogenConfig.h>

#include <h2/system/window.h>
#include <h2/ui/userInterfaceManager.h>
#include <h2/renderer/renderer.h>
#include <h2/assets/assetRegistry.h>
#include <h2/renderer/renderScene.h>
#include <h2/core/timer.h>

namespace Hydrogen
{
    class ApplicationRuntime
    {
    public:
        explicit ApplicationRuntime(const Config& hydrogenConfig);
        virtual ~ApplicationRuntime() = default;
        ApplicationRuntime(const ApplicationRuntime&) = delete;
        ApplicationRuntime& operator=(const ApplicationRuntime&) = delete;
        ApplicationRuntime(ApplicationRuntime&&) noexcept = default;
        ApplicationRuntime& operator=(ApplicationRuntime&&) noexcept = default;

        int32 Run();

    protected:
        virtual void OnInitialize() {};
        virtual void OnUpdate(float32 deltaTime) {};
        virtual void OnInput(const InputEvent& event) {}
        virtual void OnShutdown() {};

        virtual const Scene& GetScene() const = 0;
        virtual Scene& GetScene() = 0;

        const Window& GetWindow() const noexcept
        {
            return m_window;
        }

        AssetRegistry& GetAssetRegistry() noexcept
        {
            return m_assetRegistry;
        }
        const AssetRegistry& GetAssetRegistry() const noexcept
        {
            return m_assetRegistry;
        }

        UserInterfaceManager& GetUi() noexcept
        {
            return m_userInterfaceManager;
        }

        const Renderer& GetRenderer() const noexcept
        {
            return m_renderer;
        }

        Window m_window{};
        UserInterfaceManager m_userInterfaceManager{};
        AssetRegistry m_assetRegistry{};
        Renderer m_renderer{};

    private:
        void DispatchInputEvents();

        Config m_hydrogenConfig{};
        RuntimePaths m_paths{};

        Timer m_frameTimer{};
        Timer m_totalTimer{};
    };
} // namespace Hydrogen
