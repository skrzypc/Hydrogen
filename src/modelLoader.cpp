#include <cmath>
#include <filesystem>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include "modelLoader.h"
#include "verifier.h"
#include "logger.h"

namespace Hydrogen
{
    static void GenerateTangents(Mesh& mesh)
    {
        std::vector<DirectX::XMFLOAT3> tangentAccum(mesh.positions.size(), DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f});
        std::vector<DirectX::XMFLOAT3> bitangentAccum(mesh.positions.size(), DirectX::XMFLOAT3{0.0f, 0.0f, 0.0f});

        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const uint32 i0 = mesh.indices[i];
            const uint32 i1 = mesh.indices[i + 1];
            const uint32 i2 = mesh.indices[i + 2];

            const DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&mesh.positions[i0]);
            const DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&mesh.positions[i1]);
            const DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&mesh.positions[i2]);

            const DirectX::XMFLOAT2& uv0 = mesh.uvs[i0];
            const DirectX::XMFLOAT2& uv1 = mesh.uvs[i1];
            const DirectX::XMFLOAT2& uv2 = mesh.uvs[i2];

            const DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(p1, p0);
            const DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(p2, p0);

            const float32 deltaU1 = uv1.x - uv0.x;
            const float32 deltaV1 = uv1.y - uv0.y;
            const float32 deltaU2 = uv2.x - uv0.x;
            const float32 deltaV2 = uv2.y - uv0.y;

            const float32 determinant = deltaU1 * deltaV2 - deltaU2 * deltaV1;
            if (std::abs(determinant) < 1e-8f)
            {
                continue; // Degenerate UVs — leave this triangle's contribution out.
            }
            const float32 invDeterminant = 1.0f / determinant;

            const DirectX::XMVECTOR faceTangent = DirectX::XMVectorScale(
                DirectX::XMVectorSubtract(DirectX::XMVectorScale(edge1, deltaV2), DirectX::XMVectorScale(edge2, deltaV1)),
                invDeterminant);
            const DirectX::XMVECTOR faceBitangent = DirectX::XMVectorScale(
                DirectX::XMVectorSubtract(DirectX::XMVectorScale(edge2, deltaU1), DirectX::XMVectorScale(edge1, deltaU2)),
                invDeterminant);

            for (uint32 vertexIndex : {i0, i1, i2})
            {
                DirectX::XMStoreFloat3(&tangentAccum[vertexIndex],
                                       DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&tangentAccum[vertexIndex]), faceTangent));
                DirectX::XMStoreFloat3(
                    &bitangentAccum[vertexIndex],
                    DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&bitangentAccum[vertexIndex]), faceBitangent));
            }
        }

        mesh.tangents.resize(mesh.positions.size());
        for (uint32 vertexIndex = 0; vertexIndex < static_cast<uint32>(mesh.positions.size()); ++vertexIndex)
        {
            const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&mesh.normals[vertexIndex]);
            DirectX::XMVECTOR tangent = DirectX::XMLoadFloat3(&tangentAccum[vertexIndex]);
            const DirectX::XMVECTOR bitangent = DirectX::XMLoadFloat3(&bitangentAccum[vertexIndex]);

            // Gram-Schmidt orthogonalize against the vertex normal.
            tangent = DirectX::XMVectorSubtract(tangent, DirectX::XMVectorScale(normal, DirectX::XMVectorGetX(DirectX::XMVector3Dot(normal, tangent))));

            DirectX::XMFLOAT3 tangentXyz{1.0f, 0.0f, 0.0f};
            if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(tangent)) > 1e-12f)
            {
                DirectX::XMStoreFloat3(&tangentXyz, DirectX::XMVector3Normalize(tangent));
                tangent = DirectX::XMLoadFloat3(&tangentXyz);
            }

            const float32 handedness =
                DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) < 0.0f
                    ? -1.0f
                    : 1.0f;

            mesh.tangents[vertexIndex] = {tangentXyz.x, tangentXyz.y, tangentXyz.z, handedness};
        }
    }

    static Mesh ExtractPrimitive(const fastgltf::Asset& asset, const fastgltf::Primitive& prim,
                                 std::string_view meshName, uint32 primIndex)
    {
        Mesh sm{};
        sm.name = std::string(meshName);
        if (primIndex > 0)
        {
            sm.name += "_prim" + std::to_string(primIndex);
        }

        // Positions — negate Z to convert RH (glTF) → LH
        auto posIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
                                  [](const fastgltf::Attribute& a) { return a.name == "POSITION"; });
        H2_VERIFY_FATAL(posIt != prim.attributes.end(), "Mesh primitive has no POSITION attribute");
        const fastgltf::Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
        sm.positions.resize(posAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor,
                                                                  [&](fastgltf::math::fvec3 v, std::size_t i)
                                                                  { sm.positions[i] = {v.x(), v.y(), -v.z()}; });

        // Normals — negate Z to convert RH → LH
        auto nrmIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
                                  [](const fastgltf::Attribute& a) { return a.name == "NORMAL"; });
        if (nrmIt != prim.attributes.end())
        {
            const fastgltf::Accessor& nrmAccessor = asset.accessors[nrmIt->accessorIndex];
            sm.normals.resize(nrmAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, nrmAccessor,
                                                                      [&](fastgltf::math::fvec3 v, std::size_t i)
                                                                      { sm.normals[i] = {v.x(), v.y(), -v.z()}; });
        }
        else
        {
            sm.normals.resize(sm.positions.size(), {0.0f, 0.0f, 1.0f});
        }

        // UVs
        auto uvIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
                                 [](const fastgltf::Attribute& a) { return a.name == "TEXCOORD_0"; });
        if (uvIt != prim.attributes.end())
        {
            const fastgltf::Accessor& uvAccessor = asset.accessors[uvIt->accessorIndex];
            sm.uvs.resize(uvAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                asset, uvAccessor, [&](fastgltf::math::fvec2 v, std::size_t i) { sm.uvs[i] = {v.x(), v.y()}; });
        }
        else
        {
            sm.uvs.resize(sm.positions.size(), {0.0f, 0.0f});
        }

        // Indices — swap winding CCW→CW (Z negation mirrors geometry, flipping winding)
        if (prim.indicesAccessor.has_value())
        {
            const fastgltf::Accessor& idxAccessor = asset.accessors[prim.indicesAccessor.value()];
            sm.indices.resize(idxAccessor.count);
            fastgltf::iterateAccessorWithIndex<uint32>(asset, idxAccessor,
                                                       [&](uint32 idx, std::size_t i) { sm.indices[i] = idx; });
        }
        else
        {
            sm.indices.resize(sm.positions.size());
            for (uint32 i = 0; i < static_cast<uint32>(sm.positions.size()); ++i)
            {
                sm.indices[i] = i;
            }
        }

        for (std::size_t i = 0; i + 2 < sm.indices.size(); i += 3)
        {
            std::swap(sm.indices[i + 1], sm.indices[i + 2]);
        }

        // Tangents — negate Z to match the RH→LH conversion applied to positions/normals above.
        auto tanIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
                                  [](const fastgltf::Attribute& a) { return a.name == "TANGENT"; });
        if (tanIt != prim.attributes.end())
        {
            const fastgltf::Accessor& tanAccessor = asset.accessors[tanIt->accessorIndex];
            sm.tangents.resize(tanAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                asset, tanAccessor, [&](fastgltf::math::fvec4 v, std::size_t i)
                { sm.tangents[i] = {v.x(), v.y(), -v.z(), v.w()}; });
        }
        else
        {
            GenerateTangents(sm);
        }

        return sm;
    }

    static Light ExtractLight(const fastgltf::Light& modelLight)
    {
        Light light{};
        light.color = {modelLight.color.x(), modelLight.color.y(), modelLight.color.z()};
        light.intensity = static_cast<float32>(modelLight.intensity);

        switch (modelLight.type)
        {
        case fastgltf::LightType::Directional:
            light.type = eLightType::Directional;
            break;
        case fastgltf::LightType::Point:
            light.type = eLightType::Point;
            break;
        case fastgltf::LightType::Spot:
            light.type = eLightType::Spot;
            break;
        }

        if (modelLight.range.has_value())
        {
            light.range = static_cast<float32>(modelLight.range.value());
        }

        if (modelLight.innerConeAngle.has_value())
        {
            light.innerConeAngle = static_cast<float32>(modelLight.innerConeAngle.value());
        }

        if (modelLight.outerConeAngle.has_value())
        {
            light.outerConeAngle = static_cast<float32>(modelLight.outerConeAngle.value());
        }

        return light;
    }

    template <typename OptionalTextureInfoT>
    static std::optional<std::string> ResolveTexturePath(const fastgltf::Asset& asset,
                                                          const OptionalTextureInfoT& textureInfo,
                                                          const std::filesystem::path& directory)
    {
        if (!textureInfo.has_value())
        {
            return std::nullopt;
        }

        const fastgltf::Texture& texture = asset.textures[textureInfo->textureIndex];
        if (!texture.imageIndex.has_value())
        {
            return std::nullopt;
        }

        const fastgltf::Image& image = asset.images[texture.imageIndex.value()];
        if (!std::holds_alternative<fastgltf::sources::URI>(image.data))
        {
            H2_WARNING(eLogLevel::Regular, "Texture image '{}' is not URI-based, skipping", image.name);
            return std::nullopt;
        }

        const auto& uriSource = std::get<fastgltf::sources::URI>(image.data);

        std::filesystem::path ddsPath = directory / uriSource.uri.fspath();
        ddsPath.replace_extension(".dds");

        return ddsPath.string();
    }

    static Material ExtractMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material,
                                    const std::filesystem::path& directory)
    {
        Material result{};
        result.name = std::string(material.name);

        result.baseColor = {material.pbrData.baseColorFactor.x(), material.pbrData.baseColorFactor.y(),
                            material.pbrData.baseColorFactor.z()};
        result.roughness = material.pbrData.roughnessFactor;
        result.metallic = material.pbrData.metallicFactor;

        const float32 emissiveStrength = material.emissiveStrength;
        result.emissive = {material.emissiveFactor.x() * emissiveStrength,
                           material.emissiveFactor.y() * emissiveStrength,
                           material.emissiveFactor.z() * emissiveStrength};

        result.albedoTexturePath = ResolveTexturePath(asset, material.pbrData.baseColorTexture, directory);
        result.normalTexturePath = ResolveTexturePath(asset, material.normalTexture, directory);
        result.metallicRoughnessTexturePath =
            ResolveTexturePath(asset, material.pbrData.metallicRoughnessTexture, directory);

        return result;
    }

    static Transform ComposeTransforms(const Transform& parent, const Transform& local)
    {
        using namespace DirectX;

        XMVECTOR parentScale = XMLoadFloat3(&parent.scale);
        XMVECTOR parentRot = XMLoadFloat4(&parent.rotation);
        XMVECTOR parentPos = XMLoadFloat3(&parent.position);

        XMVECTOR localScale = XMLoadFloat3(&local.scale);
        XMVECTOR localRot = XMLoadFloat4(&local.rotation);
        XMVECTOR localPos = XMLoadFloat3(&local.position);

        Transform out{};
        XMStoreFloat3(&out.scale, parentScale * localScale);
        XMStoreFloat4(&out.rotation, XMQuaternionMultiply(localRot, parentRot));
        XMStoreFloat3(&out.position, parentPos + XMVector3Rotate(parentScale * localPos, parentRot));
        return out;
    }

    static void FlattenNodes(const fastgltf::Asset& asset, std::size_t nodeIndex, uint32 parentIndex,
                             const Transform& parentWorldTransform, Model& model)
    {
        const fastgltf::Node& node = asset.nodes[nodeIndex];
        uint32 currentIndex = static_cast<uint32>(model.nodes.size());

        ModelNode mn{};
        mn.name = std::string(node.name);
        mn.parentIndex = parentIndex;

        Transform localTransform{};
        // node.transform is a std::variant<TRS, fmat4x4>
        // DecomposeNodeMatrices option ensures it's always TRS
        if (std::holds_alternative<fastgltf::TRS>(node.transform))
        {
            const auto& trs = std::get<fastgltf::TRS>(node.transform);
            // Convert RH → LH: negate Z position, negate qx/qy of rotation
            localTransform.position = {trs.translation.x(), trs.translation.y(), -trs.translation.z()};
            localTransform.rotation = {-trs.rotation.x(), -trs.rotation.y(), trs.rotation.z(), trs.rotation.w()};
            localTransform.scale = {trs.scale.x(), trs.scale.y(), trs.scale.z()};
        }

        Transform worldTransform = ComposeTransforms(parentWorldTransform, localTransform);
        mn.localTransform = worldTransform;

        if (node.lightIndex.has_value())
        {
            mn.lightIndex = static_cast<uint32>(node.lightIndex.value());
        }

        if (node.meshIndex.has_value())
        {
            const fastgltf::Mesh& mesh = asset.meshes[node.meshIndex.value()];
            for (uint32 primitiveIndex = 0; primitiveIndex < static_cast<uint32>(mesh.primitives.size());
                 ++primitiveIndex)
            {
                const fastgltf::Primitive& primitive = mesh.primitives[primitiveIndex];

                uint32 meshIdx = static_cast<uint32>(model.meshes.size());
                model.meshes.push_back(ExtractPrimitive(asset, primitive, mesh.name, primitiveIndex));

                std::optional<uint32> materialIndex{};
                if (primitive.materialIndex.has_value())
                {
                    materialIndex = static_cast<uint32>(primitive.materialIndex.value());
                }

                if (materialIndex.value() == 17)
                {
                    int dbg = 4;
                }

                if (primitiveIndex == 0)
                {
                    mn.meshIndex = meshIdx;
                    mn.materialIndex = materialIndex;
                    model.nodes.push_back(std::move(mn));
                }
                else
                {
                    // Extra primitives get their own nodes as children
                    ModelNode extra{};
                    extra.name = model.meshes.back().name;
                    extra.parentIndex = currentIndex;
                    extra.meshIndex = meshIdx;
                    extra.materialIndex = materialIndex;
                    extra.localTransform = worldTransform;
                    model.nodes.push_back(std::move(extra));
                }
            }

            if (mesh.primitives.empty())
            {
                model.nodes.push_back(std::move(mn));
            }
        }
        else
        {
            model.nodes.push_back(std::move(mn));
        }

        for (std::size_t childIndex : node.children)
        {
            FlattenNodes(asset, childIndex, currentIndex, worldTransform, model);
        }
    }

    Model ModelLoader::Load(std::string_view path)
    {
        std::filesystem::path filePath(path);
        std::filesystem::path directory = filePath.parent_path();

        fastgltf::Parser parser(static_cast<fastgltf::Extensions>(std::numeric_limits<uint64>::max()));

        auto mappedFile = fastgltf::MappedGltfFile::FromPath(filePath);
        H2_VERIFY_FATAL(mappedFile.error() == fastgltf::Error::None, "Failed to open file: {}", path);

        auto result =
            parser.loadGltf(mappedFile.get(), directory,
                            fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices |
                                fastgltf::Options::GenerateMeshIndices);

        H2_VERIFY_FATAL(result.error() == fastgltf::Error::None, "Failed to parse glTF: {}", path);

        fastgltf::Asset& asset = result.get();

        Model model{};
        model.name = filePath.stem().string();

        // Light indices are kept in step with the glTF asset so nodes can reference them directly,
        // and so lights shared by several nodes stay shared.
        model.lights.reserve(asset.lights.size());
        for (const fastgltf::Light& light : asset.lights)
        {
            model.lights.push_back(ExtractLight(light));
        }

        // Material indices are kept in step with the glTF asset so primitives can
        // reference them directly, and so materials shared by several primitives stay shared.
        model.materials.reserve(asset.materials.size());
        for (const fastgltf::Material& material : asset.materials)
        {
            model.materials.push_back(ExtractMaterial(asset, material, directory));
        }

        std::size_t sceneIndex = 0;
        if (asset.defaultScene.has_value())
        {
            sceneIndex = asset.defaultScene.value();
        }

        if (!asset.scenes.empty())
        {
            const fastgltf::Scene& scene = asset.scenes[sceneIndex];
            for (std::size_t nodeIndex : scene.nodeIndices)
            {
                FlattenNodes(asset, nodeIndex, std::numeric_limits<uint32>::max(), Transform{}, model);
            }
        }

        H2_INFO(eLogLevel::Minimal, "Loaded model '{}': {} meshes, {} lights, {} materials, {} nodes", model.name,
                model.meshes.size(), model.lights.size(), model.materials.size(), model.nodes.size());
        return model;
    }
} // namespace Hydrogen
