#pragma once

#include <DirectXMath.h>
#include <string>

#include "mesh.h"

namespace Hydrogen::Primitives
{
    inline Mesh BuildBox(DirectX::XMFLOAT3 halfExtents, std::string name = "Box")
    {
        const float hx = halfExtents.x;
        const float hy = halfExtents.y;
        const float hz = halfExtents.z;

        Mesh mesh;
        mesh.name = std::move(name);
        mesh.positions.reserve(24);
        mesh.normals.reserve(24);
        mesh.uvs.reserve(24);
        mesh.indices.reserve(36);

        struct Face
        {
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT3 v0, v1, v2, v3;
        };

        const Face faces[6] =
        {
            // +X
            { { 1, 0, 0 },  { hx, -hy, -hz }, { hx,  hy, -hz }, { hx,  hy,  hz }, { hx, -hy,  hz } },
            // -X
            { { -1, 0, 0 }, { -hx, -hy,  hz }, { -hx,  hy,  hz }, { -hx,  hy, -hz }, { -hx, -hy, -hz } },
            // +Y
            { { 0, 1, 0 },  { -hx,  hy,  hz }, {  hx,  hy,  hz }, {  hx,  hy, -hz }, { -hx,  hy, -hz } },
            // -Y
            { { 0, -1, 0 }, { -hx, -hy, -hz }, {  hx, -hy, -hz }, {  hx, -hy,  hz }, { -hx, -hy,  hz } },
            // +Z
            { { 0, 0, 1 },  {  hx, -hy,  hz }, {  hx,  hy,  hz }, { -hx,  hy,  hz }, { -hx, -hy,  hz } },
            // -Z
            { { 0, 0, -1 }, { -hx, -hy, -hz }, { -hx,  hy, -hz }, {  hx,  hy, -hz }, {  hx, -hy, -hz } },
        };

        for (uint32 faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            const Face& face = faces[faceIndex];
            const uint32 baseVertex = static_cast<uint32>(mesh.positions.size());

            mesh.positions.push_back(face.v0);
            mesh.positions.push_back(face.v1);
            mesh.positions.push_back(face.v2);
            mesh.positions.push_back(face.v3);

            for (uint32 i = 0; i < 4; ++i)
            {
                mesh.normals.push_back(face.normal);
            }

            mesh.uvs.push_back({ 0.0f, 1.0f });
            mesh.uvs.push_back({ 0.0f, 0.0f });
            mesh.uvs.push_back({ 1.0f, 0.0f });
            mesh.uvs.push_back({ 1.0f, 1.0f });

            mesh.indices.push_back(baseVertex + 0);
            mesh.indices.push_back(baseVertex + 1);
            mesh.indices.push_back(baseVertex + 2);
            mesh.indices.push_back(baseVertex + 0);
            mesh.indices.push_back(baseVertex + 2);
            mesh.indices.push_back(baseVertex + 3);
        }

        return mesh;
    }
}
