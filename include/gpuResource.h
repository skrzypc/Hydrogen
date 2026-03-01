#pragma once

#include <d3d12.h>
#include <wrl.h>

namespace Hydrogen
{
    class GpuResource
    {
    public:
		GpuResource() = default;
		~GpuResource() = default;
		GpuResource(const GpuResource&) = delete;
		GpuResource& operator=(const GpuResource&) = delete;
		GpuResource(GpuResource&&) noexcept = default;
		GpuResource& operator=(GpuResource&&) noexcept = default;

		void AttachResource(ID3D12Resource* pResource) { m_pResource.Attach(pResource); }   

        ID3D12Resource* GetResource() const { return m_pResource.Get(); }
		ID3D12Resource** GetResourceAddress() { return m_pResource.GetAddressOf(); }

        D3D12_RESOURCE_STATES GetState() const { return m_state; }
        void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }

    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource = nullptr;
        D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COMMON;
    };
}