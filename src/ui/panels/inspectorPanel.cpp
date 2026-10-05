
#include <algorithm>
#include <cmath>
#include <filesystem>

#include <imgui.h>

#include "ui/uiContext.h"
#include "ui/panels.h"

#include "scene.h"
#include "assetRegistry.h"

#include "components/transformComponent.h"
#include "components/lightComponent.h"
#include "components/cameraComponent.h"
#include "components/materialComponent.h"

#include "hydrogenMath.h"

namespace Hydrogen
{
    void InspectorPanel::Draw(UiContext& context)
    {
        ImGui::Begin(GetName());

        if (!context.selection.IsValid())
        {
            ImGui::TextDisabled("No entity selected.");
            ImGui::End();
            return;
        }

        Scene& scene = *context.pScene;

        if (TransformComponent* pTransformComponent = scene.transforms.Get(context.selection))
        {
            const bool selectionChanged = context.selection != m_lastSelection;
            const bool rotationChangedExternally =
                !QuaternionEquals(pTransformComponent->transform.rotation, m_lastKnownRotation);
            if (selectionChanged || rotationChangedExternally)
            {
                const Vector3 euler = Quaternion(pTransformComponent->transform.rotation).ToEuler();
                m_cachedEulerDeg = {ToDegrees(euler.x), ToDegrees(euler.y), ToDegrees(euler.z)};
            }

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawTransformEditor(pTransformComponent->transform);
            }

            m_lastKnownRotation = pTransformComponent->transform.rotation;
        }

        m_lastSelection = context.selection;

        if (LightComponent* pLight = scene.lights.Get(context.selection))
        {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawLightEditor(pLight->light);
            }
        }

        if (CameraComponent* pCamera = scene.cameras.Get(context.selection))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawCameraEditor(*pCamera);
            }
        }

        if (const MaterialComponent* pMaterialComponent = scene.materials.Get(context.selection))
        {
            const MaterialHandle materialHandle = pMaterialComponent->material;
            if (const Material* pMaterial = context.pAssetRegistry->GetMaterial(materialHandle);
                pMaterial != nullptr && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                Material editedMaterial = *pMaterial;
                if (DrawMaterialEditor(editedMaterial))
                {
                    context.pAssetRegistry->UpdateMaterial(materialHandle, std::move(editedMaterial));
                }
            }
        }

        ImGui::End();
    }

    void InspectorPanel::DrawTransformEditor(Transform& transform)
    {
        ImGui::DragFloat3("Position (m)", &transform.position.x, 0.05f);

        if (ImGui::DragFloat3("Rotation (deg)", &m_cachedEulerDeg.x, 0.5f))
        {
            m_cachedEulerDeg.x = WrapDegrees(m_cachedEulerDeg.x);
            m_cachedEulerDeg.y = WrapDegrees(m_cachedEulerDeg.y);
            m_cachedEulerDeg.z = WrapDegrees(m_cachedEulerDeg.z);

            XMStoreFloat4(&transform.rotation, Quaternion::CreateFromYawPitchRoll(ToRadians(m_cachedEulerDeg.y),
                                                                                  ToRadians(m_cachedEulerDeg.x),
                                                                                  ToRadians(m_cachedEulerDeg.z)));
        }

        ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f, 0.001f, 1000.0f);
    }

    void InspectorPanel::DrawLightEditor(Light& light)
    {
        int32 typeIndex = static_cast<int32>(light.type);
        if (ImGui::Combo("Type", &typeIndex, "Directional\0Point\0Spot\0"))
        {
            light.type = static_cast<eLightType>(typeIndex);
        }

        ImGui::ColorEdit3("Color", &light.color.x);

        if (light.type == eLightType::Directional)
        {

            // TODO: this range is tuned to look right at the default exposure, not real lux magnitudes.
            ImGui::SliderFloat("Intensity (lux)", &light.intensity, 0.0f, 50.0f, "%.2f");
        }
        else
        {
            // UI authors total luminous flux (lumens); the shader needs candela (lm/sr), so convert
            // through the solid angle the light actually emits into (full sphere vs. its cone).
            const float32 solidAngleSr =
                (light.type == eLightType::Spot)
                    ? 2.0f * DirectX::XM_PI * (1.0f - std::cos(light.outerConeAngle.value_or(ToRadians(45.0f))))
                    : 4.0f * DirectX::XM_PI;

            float32 lumens = light.intensity * solidAngleSr;
            if (ImGui::SliderFloat("Intensity (lm)", &lumens, 0.0f, 5000.0f, "%.0f"))
            {
                light.intensity = lumens / solidAngleSr;
            }
        }

        if (light.type != eLightType::Directional)
        {
            float32 range = light.range.value_or(0.0f);
            if (ImGui::SliderFloat("Range", &range, 0.0f, 100.0f, "%.2f m"))
            {
                light.range = range;
            }
        }

        if (light.type == eLightType::Spot)
        {
            float32 innerDeg = ToDegrees(light.innerConeAngle.value_or(0.0f));
            float32 outerDeg = ToDegrees(light.outerConeAngle.value_or(ToRadians(45.0f)));
            // Inner must stay <= outer or the shader's cone falloff inverts.
            if (ImGui::SliderFloat("Inner cone", &innerDeg, 0.0f, 89.0f, "%.1f deg"))
            {
                light.innerConeAngle = ToRadians(std::min(innerDeg, outerDeg));
            }
            if (ImGui::SliderFloat("Outer cone", &outerDeg, 0.0f, 89.0f, "%.1f deg"))
            {
                light.outerConeAngle = ToRadians(std::max(outerDeg, innerDeg));
            }
        }
    }

    void InspectorPanel::DrawCameraEditor(CameraComponent& camera)
    {
        ImGui::SliderFloat("FOV Y", &camera.fovYDeg, 10.0f, 120.0f, "%.1f deg");
        ImGui::DragFloat("Near Z", &camera.nearZ, 0.001f, 0.001f, camera.farZ - 0.01f, "%.3f");
        ImGui::DragFloat("Far Z", &camera.farZ, 0.1f, camera.nearZ + 0.01f, 10000.0f, "%.1f");
        ImGui::SliderFloat("Exposure", &camera.exposure, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }

    bool InspectorPanel::DrawMaterialEditor(Material& material)
    {
        ImGui::LabelText("Name", "%s", material.name.c_str());

        ImGui::SeparatorText("Attributes");
        bool changed = false;
        changed |= ImGui::ColorEdit3("Albedo", &material.baseColor.x);
        changed |= ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f, "%.3f");
        changed |= ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f, "%.3f");
        changed |= ImGui::ColorEdit3("Emissive", &material.emissive.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

        const auto drawTexturePathRow = [](const char* pLabel, const std::optional<std::string>& texturePath)
        {
            if (!texturePath.has_value())
            {
                ImGui::LabelText(pLabel, "None");
                return;
            }

            const auto fileName = std::filesystem::path(*texturePath).filename().string();
            ImGui::LabelText(pLabel, "%s", fileName.c_str());
        };

        ImGui::SeparatorText("Textures");
        drawTexturePathRow("Albedo", material.albedoTexturePath);
        drawTexturePathRow("Normal", material.normalTexturePath);
        drawTexturePathRow("Metallic / Roughness", material.metallicRoughnessTexturePath);

        return changed;
    }
} // namespace Hydrogen
