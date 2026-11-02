#include <h2/ui/panels.h>

#include <string>
#include <vector>

#include <imgui.h>

#include <h2/core/verifier.h>
#include <h2/ui/userInterfaceContext.h>
#include <h2/scene/scene.h>
#include <h2/assets/assetRegistry.h>
#include <h2/scene/components/meshComponent.h>

namespace Hydrogen
{
    namespace
    {
        template <typename NameFn>
        void DrawSelectableList(const char* headerLabel, const std::vector<Entity>& entities, Entity& selection,
                                NameFn&& nameFn)
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
    } // namespace

    void ScenePanel::Draw(UserInterfaceContext& context)
    {
        H2_VERIFY(context.pScene != nullptr && context.pAssetRegistry != nullptr && context.pSelection != nullptr,
                  "Scene, asset registry or selection is missing from the UI context!");

        ImGui::Begin(GetName());

        Scene& scene = *context.pScene;
        AssetRegistry& assetRegistry = *context.pAssetRegistry;
        Entity& selection = *context.pSelection;

        DrawSelectableList("Meshes", scene.meshes.GetEntities(), selection,
                           [&](uint32 index, Entity entity)
                           {
                               const MeshComponent& meshComponent = scene.meshes.GetAll()[index];
                               if (const MeshMetadata* pMetadata = assetRegistry.GetMeshMetadata(meshComponent.mesh);
                                   pMetadata && !pMetadata->name.empty())
                               {
                                   return pMetadata->name;
                               }
                               return "Mesh " + std::to_string(entity.id);
                           });

        DrawSelectableList("Lights", scene.lights.GetEntities(), selection,
                           [](uint32 index, Entity entity) { return "Light " + std::to_string(entity.id); });

        DrawSelectableList("Cameras", scene.cameras.GetEntities(), selection,
                           [](uint32 index, Entity entity) { return "Camera " + std::to_string(entity.id); });

        ImGui::End();
    }
} // namespace Hydrogen
