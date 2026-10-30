#pragma once

#include <h2/core/basicTypes.h>

#include <h2/hydrogenConfig.h>

#include <h2/system/window.h>
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
        virtual void OnInput() {}
        virtual void OnShutdown() {};

        virtual const Scene& GetScene() const = 0;

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

        const Renderer& GetRenderer() const noexcept
        {
            return m_renderer;
        }

        Window m_window{};
        AssetRegistry m_assetRegistry{};
        Renderer m_renderer{};

    private:
        Config m_hydrogenConfig{};

        Timer m_frameTimer{};
        uint64 m_frameNumber = 0ull;
    };
} // namespace Hydrogen
