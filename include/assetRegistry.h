#pragma once

#include <vector>

#include "basicTypes.h"
#include "mesh.h"
#include "assetUploadQueue.h"

namespace Hydrogen
{
    class AssetRegistry
    {
    public:
        MeshHandle RegisterMesh(MeshMetadata&& metadata, Mesh&& mesh)
        {
            MeshHandle handle{ static_cast<uint32>(m_meshMetadata.size()) };

            m_meshMetadata.push_back(std::move(metadata));
            m_uploadQueue.Push({ handle, m_meshMetadata.back(), std::move(mesh) });

            return handle;
        }

        const MeshMetadata* GetMeshMetadata(MeshHandle handle) const
        {
            if (handle.id >= m_meshMetadata.size())
            {
                return nullptr;
            }
            return &m_meshMetadata[handle.id];
        }

        AssetUploadQueue& GetUploadQueue() { return m_uploadQueue; }

    private:
        std::vector<MeshMetadata> m_meshMetadata{};

        AssetUploadQueue m_uploadQueue{};
    };
}
