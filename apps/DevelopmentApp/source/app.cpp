
#include <app.h>

Application::Application(const Config& appConfig) : ApplicationRuntime(appConfig)
{

}

void Application::OnInitialize() {}

void Application::OnUpdate(float32 deltaTime) {}

void Application::OnShutdown() {}

const Hydrogen::Scene& Application::GetScene() const
{
    return m_scene;
}

