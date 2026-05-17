#include <filesystem>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include "modelLoader.h"
#include "verifier.h"
#include "logger.h"

namespace Hydrogen
{
    static Mesh ExtractPrimitive(const fastgltf::Asset& asset, const fastgltf::Primitive& prim, std::string_view meshName, uint32 primIndex)
    {
        Mesh sm{};
        sm.name = std::string(meshName);
        if (primIndex > 0)
        {
            sm.name += "_prim" + std::to_string(primIndex);
        }

        // Positions
        auto posIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
            [](const fastgltf::Attribute& a) { return a.name == "POSITION"; });
        H2_VERIFY_FATAL(posIt != prim.attributes.end(), "Mesh primitive has no POSITION attribute");
        const fastgltf::Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
        sm.positions.resize(posAccessor.count);
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor,
            [&](fastgltf::math::fvec3 v, std::size_t i) {
                sm.positions[i] = { v.x(), v.y(), v.z() };
            });

        // Normals
        auto nrmIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
            [](const fastgltf::Attribute& a) { return a.name == "NORMAL"; });
        if (nrmIt != prim.attributes.end())
        {
            const fastgltf::Accessor& nrmAccessor = asset.accessors[nrmIt->accessorIndex];
            sm.normals.resize(nrmAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, nrmAccessor,
                [&](fastgltf::math::fvec3 v, std::size_t i) {
                    sm.normals[i] = { v.x(), v.y(), v.z() };
                });
        }
        else
        {
            sm.normals.resize(sm.positions.size(), { 0.0f, 0.0f, 1.0f });
        }

        // UVs
        auto uvIt = std::find_if(prim.attributes.begin(), prim.attributes.end(),
            [](const fastgltf::Attribute& a) { return a.name == "TEXCOORD_0"; });
        if (uvIt != prim.attributes.end())
        {
            const fastgltf::Accessor& uvAccessor = asset.accessors[uvIt->accessorIndex];
            sm.uvs.resize(uvAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, uvAccessor,
                [&](fastgltf::math::fvec2 v, std::size_t i) {
                    sm.uvs[i] = { v.x(), v.y() };
                });
        }
        else
        {
            sm.uvs.resize(sm.positions.size(), { 0.0f, 0.0f });
        }

        // Indices
        if (prim.indicesAccessor.has_value())
        {
            const fastgltf::Accessor& idxAccessor = asset.accessors[prim.indicesAccessor.value()];
            sm.indices.resize(idxAccessor.count);
            fastgltf::iterateAccessorWithIndex<uint32>(asset, idxAccessor,
                [&](uint32 idx, std::size_t i) {
                    sm.indices[i] = idx;
                });
        }
        else
        {
            sm.indices.resize(sm.positions.size());
            for (uint32 i = 0; i < static_cast<uint32>(sm.positions.size()); ++i)
            {
                sm.indices[i] = i;
            }
        }

        return sm;
    }

    static void FlattenNodes(const fastgltf::Asset& asset, std::size_t nodeIndex, uint32 parentIndex, Model& model)
    {
        const fastgltf::Node& node = asset.nodes[nodeIndex];
        uint32 currentIndex = static_cast<uint32>(model.nodes.size());

        ModelNode mn{};
        mn.name = std::string(node.name);
        mn.parentIndex = parentIndex;

        // node.transform is a std::variant<TRS, fmat4x4>
        // DecomposeNodeMatrices option ensures it's always TRS
        if (std::holds_alternative<fastgltf::TRS>(node.transform))
        {
            const auto& trs = std::get<fastgltf::TRS>(node.transform);
            mn.localTransform.position = { trs.translation.x(), trs.translation.y(), trs.translation.z() };
            mn.localTransform.rotation = { trs.rotation.x(), trs.rotation.y(), trs.rotation.z(), trs.rotation.w() };
            mn.localTransform.scale = { trs.scale.x(), trs.scale.y(), trs.scale.z() };
        }

        if (node.meshIndex.has_value())
        {
            const fastgltf::Mesh& mesh = asset.meshes[node.meshIndex.value()];
            for (uint32 p = 0; p < static_cast<uint32>(mesh.primitives.size()); ++p)
            {
                uint32 meshIdx = static_cast<uint32>(model.meshes.size());
                model.meshes.push_back(ExtractPrimitive(asset, mesh.primitives[p], mesh.name, p));

                if (p == 0)
                {
                    mn.meshIndex = meshIdx;
                    model.nodes.push_back(std::move(mn));
                }
                else
                {
                    // Extra primitives get their own nodes as children
                    ModelNode extra{};
                    extra.name = model.meshes.back().name;
                    extra.parentIndex = currentIndex;
                    extra.meshIndex = meshIdx;
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
            FlattenNodes(asset, childIndex, currentIndex, model);
        }
    }

    Model ModelLoader::Load(std::string_view path)
    {
        std::filesystem::path filePath(path);
        std::filesystem::path directory = filePath.parent_path();

        fastgltf::Parser parser;

        auto mappedFile = fastgltf::MappedGltfFile::FromPath(filePath);
        H2_VERIFY_FATAL(mappedFile.error() == fastgltf::Error::None, "Failed to open file: {}", path);

        auto result = parser.loadGltf(
            mappedFile.get(),
            directory,
            fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::DecomposeNodeMatrices |
            fastgltf::Options::GenerateMeshIndices
        );

        H2_VERIFY_FATAL(result.error() == fastgltf::Error::None, "Failed to parse glTF: {}", path);

        fastgltf::Asset& asset = result.get();

        Model model{};
        model.name = filePath.stem().string();

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
                FlattenNodes(asset, nodeIndex, UINT32_MAX, model);
            }
        }

        H2_INFO(eLogLevel::Minimal, "Loaded model '{}': {} meshes, {} nodes", model.name, model.meshes.size(), model.nodes.size());
        return model;
    }
}
