//#pragma once
//
//#include <h2/core/basicTypes.h>
//#include <h2/system/window.h>
//#include <h2/renderer/renderer.h>
//#include <h2/scene/scene.h>
//#include <h2/assets/assetRegistry.h>
//#include <h2/scene/entity.h>
//#include <h2/core/timer.h>
//#include <h2/ui/debugUi.h>
//
//namespace Hydrogen
//{
//    class Engine
//    {
//    public:
//        Engine() = default;
//        ~Engine() = default;
//        Engine(const Engine&) = delete;
//        const Engine& operator=(const Engine&) = delete;
//        Engine(Engine&&) noexcept = default;
//        Engine& operator=(Engine&&) noexcept = default;
//
//        // TODO: Parse command line arguments before passing them to the engine.
//        int32 Run(LPSTR commandLineArgs);
//
//    private:
//        Window m_window{};
//        Renderer m_renderer{};
//        Scene m_scene{};
//        AssetRegistry m_assetRegistry{};
//        DebugUi m_debugUi{};
//
//        Entity m_activeCamera{};
//        float32 m_cameraSpeed = 15.0f;
//        Timer m_frameTimer{};
//        Timer m_appTimer{};
//    };
//} // namespace Hydrogen
