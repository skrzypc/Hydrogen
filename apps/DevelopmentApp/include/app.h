#pragma once

#include <h2/applicationRuntime.h>
#include <h2/scene/scene.h>

class Application : public Hydrogen::ApplicationRuntime
{
public:
    struct Config : public Hydrogen::Config
    {
        int example = 5;
    };
public:
    explicit Application(const Config& appConfig);
    ~Application() = default;
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;
    Application(Application&&) noexcept = default;
    Application& operator=(Application&&) noexcept = default;

    void OnInitialize() override;
    void OnUpdate(float32 deltaTime) override;
    void OnShutdown() override;

    const Hydrogen::Scene& GetScene() const override;

private:
    Hydrogen::Scene m_scene{};
};
