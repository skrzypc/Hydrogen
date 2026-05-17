#pragma once

#include <DirectXMath.h>

namespace Hydrogen
{
    struct Transform
    {
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; // quaternion (x,y,z,w)
        DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

        DirectX::XMMATRIX GetWorldMatrix() const
        {
            DirectX::XMMATRIX s = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&scale));
            DirectX::XMMATRIX r = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rotation));
            DirectX::XMMATRIX t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&position));
            return s * r * t;
        }
    };
}
