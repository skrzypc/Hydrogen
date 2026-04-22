#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "verifier.h"

namespace Hydrogen
{
	class CopyContext
	{
	public:
		CopyContext(ID3D12GraphicsCommandList10* pList)
			: m_pCommandList(pList)
		{}
		~CopyContext()
		{
			H2_VERIFY(m_pCommandList == nullptr, "CopyContext destroyed without being executed!");
		}
		CopyContext(const CopyContext&) = delete;
		CopyContext& operator=(const CopyContext&) = delete;
		CopyContext(CopyContext&&) noexcept = default;
		CopyContext& operator=(CopyContext&&) noexcept = default;

		ID3D12GraphicsCommandList10* CmdList() const { return m_pCommandList.Get(); }

	private:
		friend class GpuDevice;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_pCommandList = nullptr;
	};
}
