#pragma once

#include "entity.h"
#include "componentStore.h"
#include "components/transformComponent.h"
#include "components/meshComponent.h"
#include "components/hierarchyComponent.h"
#include "components/cameraComponent.h"
#include "basicTypes.h"

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
        ComponentStore<HierarchyComponent> hierarchy;
        ComponentStore<CameraComponent> cameras;

    private:
        uint32 m_nextId = 0;
    };
}
