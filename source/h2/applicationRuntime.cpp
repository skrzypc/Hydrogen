
#include "h2/applicationRuntime.h"

namespace Hydrogen
{
    ApplicationRuntime::ApplicationRuntime(const Config& hydrogenConfig) : m_hydrogenConfig(hydrogenConfig)
    {
        Logger::Initialize(hydrogenConfig.logLevel);
    }

    int32 ApplicationRuntime::Run()
    {
        if (m_hydrogenConfig.waitForDebugger)
        {
            while (!::IsDebuggerPresent())
            {
                ::Sleep(1000);

                H2_INFO(eLogLevel::Verbose, "Waiting for debugger.");
            }
        }

        m_window.Create(m_hydrogenConfig.windowWidth, m_hydrogenConfig.windowHeight, m_hydrogenConfig.windowTitle);

        m_userInterfaceManager.Initialize(m_window.GetHandle(), m_hydrogenConfig.uiSettingsPath);

        m_renderer.Initialize(m_window.GetWidth(), m_window.GetHeight(), m_window.GetHandle());
        // TODO: Avoid that?
        m_renderer.SetUploadQueue(&m_assetRegistry.GetUploadQueue());

        OnInitialize();

        int32 returnCode = 0;
        while (true)
        {
            const float64 time = m_totalTimer.GetSeconds();
            const float64 deltaSeconds = m_frameTimer.GetSeconds();
            m_frameTimer.Mark();

            if (const auto ecode = m_window.ProcessMessages())
            {
                OnShutdown();

                returnCode = *ecode;
                break;
            }
            DispatchInputEvents();

            m_userInterfaceManager.BeginFrame();

            OnUpdate(static_cast<float32>(deltaSeconds));

            UserInterfaceContext context{
                .pScene = &GetScene(),
                .pAssetRegistry = &m_assetRegistry,
                .pRenderer = &m_renderer,
                .deltaTime = static_cast<float32>(deltaSeconds),
                .time = time,
            };
            m_userInterfaceManager.Draw(context);

            m_renderer.SubmitFrame(GetScene(), m_userInterfaceManager.EndFrame(), context.rendererRequest, time,
                                   static_cast<float32>(deltaSeconds));
        }

        return returnCode;
    }

    void ApplicationRuntime::DispatchInputEvents()
    {
        for (const InputEvent& event : m_window.GetInputEvents())
        {
            if (event.type == eInputEventType::KeyDown && event.keyCode == VK_F1)
            {
                m_userInterfaceManager.SetVisible(!m_userInterfaceManager.IsVisible());
                continue;
            }

            const bool capturedByUi = event.IsKeyboardEvent() ? m_userInterfaceManager.WantsKeyboardCapture()
                                                              : m_userInterfaceManager.WantsMouseCapture();
            if (!capturedByUi)
            {
                OnInput(event);
            }
        }
    }
} // namespace Hydrogen
