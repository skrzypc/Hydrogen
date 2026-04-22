#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstring>

#include "basicTypes.h"
#include "verifier.h"
#include "rootSignature.h"
#include "uploadRingBuffer.h"

namespace Hydrogen
{
	class GraphicsContext
	{
	public:
		GraphicsContext(ID3D12GraphicsCommandList10* pList)
			: m_pCommandList(pList)
		{}
		~GraphicsContext()
		{
			H2_VERIFY(m_pCommandList == nullptr, "GraphicsContext destroyed without being executed!");
		}
		GraphicsContext(const GraphicsContext&) = delete;
		GraphicsContext& operator=(const GraphicsContext&) = delete;
		GraphicsContext(GraphicsContext&&) noexcept = default;
		GraphicsContext& operator=(GraphicsContext&&) noexcept = default;

		ID3D12GraphicsCommandList10* CmdList() const { return m_pCommandList.Get(); }

		template<typename T>
		void SetFrameData(const T& data)
		{
			auto [pCpuData, gpuAddress] = s_pUploadBuffer->Allocate(sizeof(T));
			memcpy(pCpuData, &data, sizeof(T));
			m_pCommandList->SetGraphicsRootConstantBufferView(static_cast<uint32>(eRootParam::FrameConstantBuffer), gpuAddress);
		}

		template<typename T>
		void SetPassData(const T& data)
		{
			auto [pCpuData, gpuAddress] = s_pUploadBuffer->Allocate(sizeof(T));
			memcpy(pCpuData, &data, sizeof(T));
			m_pCommandList->SetGraphicsRootConstantBufferView(static_cast<uint32>(eRootParam::PassConstantBuffer), gpuAddress);
		}

		template<typename T>
		void SetPushConstants(const T& data)
		{
			H2_VERIFY_STATIC(sizeof(T) <= RootSignature::PushConstantCount * sizeof(uint32));
			m_pCommandList->SetGraphicsRoot32BitConstants(static_cast<uint32>(eRootParam::PushConstants), sizeof(T) / sizeof(uint32), &data, 0);
		}

		inline static UploadRingBuffer* s_pUploadBuffer = nullptr;

	private:
		friend class GpuDevice;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> m_pCommandList = nullptr;
	};
}
