#pragma once

#include <vector>

#include "mesh.h"
#include "material.h"
#include "textureAsset.h"

namespace Hydrogen
{
    struct MeshUploadRequest
    {
        MeshHandle handle{};
        MeshMetadata metadata{};
        Mesh mesh{};
    };

    struct MaterialUploadRequest
    {
        MaterialHandle handle{};
        Material material{};
    };

    struct TextureUploadRequest
    {
        TextureHandle handle{};
        TextureMetadata metadata{};
        TextureData data{};
    };

    class AssetUploadQueue
    {
    public:
        void Push(MeshUploadRequest request)
        {
            // TODO: add mutex when multithreading is introduced
            m_pending.push_back(std::move(request));
        }

        std::vector<MeshUploadRequest> Drain()
        {
            // TODO: add mutex when multithreading is introduced
            std::vector<MeshUploadRequest> result{};
            result.swap(m_pending);

            return result;
        }

        void PushMaterial(MaterialUploadRequest request)
        {
            // TODO: add mutex when multithreading is introduced
            m_pendingMaterials.push_back(std::move(request));
        }

        std::vector<MaterialUploadRequest> DrainMaterials()
        {
            // TODO: add mutex when multithreading is introduced
            std::vector<MaterialUploadRequest> result{};
            result.swap(m_pendingMaterials);

            return result;
        }

        void PushTexture(TextureUploadRequest request)
        {
            // TODO: add mutex when multithreading is introduced
            m_pendingTextures.push_back(std::move(request));
        }

        std::vector<TextureUploadRequest> DrainTextures()
        {
            // TODO: add mutex when multithreading is introduced
            std::vector<TextureUploadRequest> result{};
            result.swap(m_pendingTextures);

            return result;
        }

    private:
        std::vector<MeshUploadRequest> m_pending{};
        std::vector<MaterialUploadRequest> m_pendingMaterials{};
        std::vector<TextureUploadRequest> m_pendingTextures{};
    };
} // namespace Hydrogen
