#pragma once

#include <h2/scene/entity.h>
#include <h2/scene/componentStore.h>
#include <h2/scene/components/transformComponent.h>
#include <h2/scene/components/meshComponent.h>
#include <h2/scene/components/materialComponent.h>
#include <h2/scene/components/hierarchyComponent.h>
#include <h2/scene/components/cameraComponent.h>
#include <h2/scene/components/lightComponent.h>
#include <h2/core/basicTypes.h>

namespace Hydrogen
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) noexcept = default;
        Scene& operator=(Scene&&) noexcept = default;

        Entity CreateEntity();
        void DestroyEntity(Entity entity);

        ComponentStore<TransformComponent> transforms;
        ComponentStore<MeshComponent> meshes;
        ComponentStore<MaterialComponent> materials;
        ComponentStore<HierarchyComponent> hierarchy;
        ComponentStore<CameraComponent> cameras;
        ComponentStore<LightComponent> lights;

    private:
        uint32 m_nextId = 0;
    };
} // namespace Hydrogen
