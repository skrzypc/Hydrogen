
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

        m_renderer.Initialize(m_window.GetWidth(), m_window.GetHeight(), m_window.GetHandle());
        // TODO: Avoid that?
        m_renderer.SetUploadQueue(&m_assetRegistry.GetUploadQueue());

        OnInitialize();

        int32 returnCode = 0;
        while (true)
        {
            const float64 deltaSeconds = m_frameTimer.GetSeconds();
            m_frameTimer.Mark();

            if (const auto ecode = m_window.ProcessMessages())
            {
                OnShutdown();

                returnCode = *ecode;
                break;
            }
            OnInput();

            OnUpdate(static_cast<float32>(deltaSeconds));

            m_renderer.SubmitFrame(GetScene(), m_frameNumber, static_cast<float32>(deltaSeconds));

            ++m_frameNumber;
        }

        return returnCode;
    }
} // namespace Hydrogen
