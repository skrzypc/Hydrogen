#pragma once

#include <string_view>

#include <d3d12.h>
#include <wrl.h>

namespace Hydrogen
{
	struct ResourceState
	{
		D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
		D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_NO_ACCESS;
		D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
	};

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

        void SetName(std::wstring_view name) { m_pResource->SetName(name.data()); }

        const ResourceState& GetState() const { return m_state; }
        void SetState(ResourceState state) { m_state = state; }

    protected:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource = nullptr;
        ResourceState m_state{};
    };
}