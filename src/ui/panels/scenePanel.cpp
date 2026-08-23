#include "ui/panels.h"

#include <string>
#include <vector>

#include <imgui.h>

#include "ui/uiContext.h"
#include "scene.h"
#include "assetRegistry.h"
#include "components/meshComponent.h"

namespace Hydrogen
{
    namespace
    {
        template<typename NameFn>
        void DrawSelectableList(const char* headerLabel, const std::vector<Entity>& entities, Entity& selection, NameFn&& nameFn)
        {
            if (!ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_None))
            {
                return;
            }

            for (uint32 i = 0; i < static_cast<uint32>(entities.size()); ++i)
            {
                const Entity entity = entities[i];
                const std::string label = nameFn(i, entity);
                if (ImGui::Selectable(label.c_str(), selection == entity))
                {
                    selection = entity;
                }
            }
        }
    }

    void ScenePanel::Draw(UiContext& context)
    {
        ImGui::Begin(GetName());

        Scene& scene = *context.pScene;
        AssetRegistry& assetRegistry = *context.pAssetRegistry;

        DrawSelectableList("Meshes", scene.meshes.GetEntities(), context.selection,
            [&](uint32 index, Entity entity)
            {
                const MeshComponent& meshComponent = scene.meshes.GetAll()[index];
                if (const MeshMetadata* pMetadata = assetRegistry.GetMeshMetadata(meshComponent.mesh); pMetadata && !pMetadata->name.empty())
                {
                    return pMetadata->name;
                }
                return "Mesh " + std::to_string(entity.id);
            });

        DrawSelectableList("Lights", scene.lights.GetEntities(), context.selection,
            [](uint32 index, Entity entity) { return "Light " + std::to_string(entity.id); });

        DrawSelectableList("Cameras", scene.cameras.GetEntities(), context.selection,
            [](uint32 index, Entity entity) { return "Camera " + std::to_string(entity.id); });

        ImGui::End();
    }
}
