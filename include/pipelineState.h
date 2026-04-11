#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <span>

#include "basicTypes.h"
#include "shader.h"

namespace Hydrogen
{
	class GpuDevice;

	class PipelineState
	{
	public:
		struct GraphicsDesc
		{
			const Shader* pVertexShader = nullptr;
			const Shader* pPixelShader = nullptr;
			// const Shader* pMeshShader = nullptr;  // future
			// const Shader* pAmplificationShader = nullptr;  // future

			std::span<const DXGI_FORMAT> renderTargetFormats;
			DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;

			D3D12_RASTERIZER_DESC2 rasterizerDesc = DefaultRasterizer();
			D3D12_BLEND_DESC blendDesc = DefaultBlend();
			D3D12_DEPTH_STENCIL_DESC1 depthStencilDesc = DefaultDepthStencil();

			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		};

		struct ComputeDesc
		{
			const Shader* pComputeShader = nullptr;
		};

		PipelineState() = default;
		~PipelineState() = default;
		PipelineState(const PipelineState&) = delete;
		PipelineState& operator=(const PipelineState&) = delete;
		PipelineState(PipelineState&&) noexcept = default;
		PipelineState& operator=(PipelineState&&) noexcept = default;

		void CreateGraphics(GpuDevice& device, const GraphicsDesc& desc);
		void CreateCompute(GpuDevice& device, const ComputeDesc& desc);

		ID3D12PipelineState* Get() const { return m_pDxPipelineState.Get(); }

		static D3D12_RASTERIZER_DESC2 DefaultRasterizer();
		static D3D12_BLEND_DESC DefaultBlend();
		static D3D12_DEPTH_STENCIL_DESC1 DefaultDepthStencil();

	private:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDxPipelineState = nullptr;
	};
}
