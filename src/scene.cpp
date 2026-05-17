#include "scene.h"

namespace Hydrogen
{
    Entity Scene::CreateEntity()
    {
        return Entity{ m_nextId++ };
    }

    void Scene::DestroyEntity(Entity entity)
    {
        transforms.Remove(entity);
        meshes.Remove(entity);
        hierarchy.Remove(entity);
    }
}
