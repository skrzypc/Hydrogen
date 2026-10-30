
#include <algorithm>
#include <cmath>

#include <app.h>

#include <h2/assets/modelLoader.h>
#include <h2/assets/textureLoader.h>
#include <h2/core/math.h>

using namespace Hydrogen;

Application::Application(const Config& appConfig) : ApplicationRuntime(appConfig)
{

}

void Application::OnInitialize()
{
    LoadModels();
    CreateCamera();
}

void Application::LoadModels()
{
    std::vector<Model> models{};
    models.emplace_back(ModelLoader::Load("apps/assets/AmdSponza/MainSponza.gltf"));

    auto registerTextureIfPresent = [this](const std::optional<std::string>& path) -> TextureHandle
    {
        if (!path.has_value())
        {
            return TextureHandle{};
        }
        TextureMetadata metadata{.name = *path, .path = *path};
        return GetAssetRegistry().RegisterTexture(std::move(metadata), TextureLoader::Load(*path));
    };

    for (Model& model : models)
    {
        std::vector<MeshHandle> meshHandles{};
        for (Mesh& mesh : model.meshes)
        {
            MeshMetadata metadata{.name = mesh.name};
            meshHandles.push_back(GetAssetRegistry().RegisterMesh(std::move(metadata), std::move(mesh)));
        }

        std::vector<MaterialHandle> materialHandles{};
        for (Material& material : model.materials)
        {
            material.albedoTexture = registerTextureIfPresent(material.albedoTexturePath);
            material.normalTexture = registerTextureIfPresent(material.normalTexturePath);
            material.metallicRoughnessTexture = registerTextureIfPresent(material.metallicRoughnessTexturePath);

            materialHandles.push_back(GetAssetRegistry().RegisterMaterial(std::move(material)));
        }

        for (const ModelNode& node : model.nodes)
        {
            Entity entity = m_scene.CreateEntity();
            m_scene.transforms.Add(entity, TransformComponent{node.localTransform});
            if (node.meshIndex.has_value())
            {
                m_scene.meshes.Add(entity, MeshComponent{meshHandles[*node.meshIndex]});
            }

            if (node.materialIndex.has_value())
            {
                m_scene.materials.Add(entity, MaterialComponent{materialHandles[*node.materialIndex]});
            }

            if (node.lightIndex.has_value())
            {
                m_scene.lights.Add(entity, LightComponent{model.lights[*node.lightIndex]});
            }
        }
    }
}

void Application::CreateCamera()
{
    constexpr Vector3 initialCameraPosition = {0.0f, 0.2f, -1.0f};

    m_scene.activeCamera = m_scene.CreateEntity();

    Transform cameraTransform{};
    cameraTransform.position = initialCameraPosition;
    m_scene.transforms.Add(m_scene.activeCamera, TransformComponent{cameraTransform});
    m_scene.cameras.Add(m_scene.activeCamera, CameraComponent{});
}

namespace
{
    constexpr float32 MouseSensitivity = 0.1f;
    constexpr float32 MaxPitchDegrees = 89.0f;
    constexpr float32 MinCameraSpeed = 0.5f;
    constexpr float32 MaxCameraSpeed = 20.0f;
    constexpr float32 CameraSpeedScrollStep = 0.5f;
    constexpr float32 MinFovDegrees = 10.0f;
    constexpr float32 MaxFovDegrees = 120.0f;
    constexpr float32 FovScrollStep = 2.0f;

    void ApplyMouseLook(const Window& window, Transform& transform)
    {
        if (!window.IsRightMouseDown())
        {
            return;
        }

        const Vector3 currentEuler = Quaternion(transform.rotation).ToEuler();
        const float32 pitch = std::clamp(ToDegrees(currentEuler.x) + window.GetMouseDeltaY() * MouseSensitivity,
                                         -MaxPitchDegrees, MaxPitchDegrees);
        const float32 yaw = ToDegrees(currentEuler.y) + window.GetMouseDeltaX() * MouseSensitivity;

        DirectX::XMStoreFloat4(&transform.rotation,
                               Quaternion::CreateFromYawPitchRoll(ToRadians(yaw), ToRadians(pitch), 0.0f));
    }

    Vector3 GetMoveDirection(const Window& window, const Quaternion& orientation)
    {
        const Vector3 forward = Vector3::Transform(Forward, orientation);
        const Vector3 right = Vector3::Transform(Right, orientation);

        Vector3 move = Vector3::Zero;
        if (window.IsKeyDown('W'))
        {
            move += forward;
        }
        if (window.IsKeyDown('S'))
        {
            move -= forward;
        }
        if (window.IsKeyDown('D'))
        {
            move += right;
        }
        if (window.IsKeyDown('A'))
        {
            move -= right;
        }
        if (window.IsKeyDown('E'))
        {
            move += Up;
        }
        if (window.IsKeyDown('Q'))
        {
            move -= Up;
        }
        return move;
    }
} // namespace

void Application::OnUpdate(float32 deltaTime)
{
    UpdateCamera(deltaTime);
}

void Application::UpdateCamera(float32 deltaTime)
{
    const Window& window = GetWindow();

    TransformComponent* pTransformComponent = m_scene.transforms.Get(m_scene.activeCamera);
    CameraComponent* pCameraComponent = m_scene.cameras.Get(m_scene.activeCamera);

    if (pTransformComponent != nullptr)
    {
        Transform& transform = pTransformComponent->transform;
        ApplyMouseLook(window, transform);

        Vector3 move = GetMoveDirection(window, Quaternion(transform.rotation));
        if (move.LengthSquared() > 0.0f)
        {
            move.Normalize();
            move *= m_cameraSpeed * deltaTime;
        }

        transform.position.x += move.x;
        transform.position.y += move.y;
        transform.position.z += move.z;
    }

    if (pCameraComponent == nullptr)
    {
        return;
    }

    if (const float32 scroll = window.GetScrollDelta(); scroll != 0.0f)
    {
        if (window.IsRightMouseDown())
        {
            m_cameraSpeed =
                std::clamp(m_cameraSpeed + scroll * CameraSpeedScrollStep, MinCameraSpeed, MaxCameraSpeed);
        }
        else
        {
            pCameraComponent->fovYDeg =
                std::clamp(pCameraComponent->fovYDeg - scroll * FovScrollStep, MinFovDegrees, MaxFovDegrees);
        }
    }

    if (window.IsMiddleMouseJustPressed())
    {
        pCameraComponent->fovYDeg = CameraComponent{}.fovYDeg;
    }
}

void Application::OnShutdown() {}

const Hydrogen::Scene& Application::GetScene() const
{
    return m_scene;
}

