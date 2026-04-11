#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <array>

#include "basicTypes.h"

namespace Hydrogen
{
	class GpuDevice;

	// Rigit format of the RS. 12 DWORDs in total.
	enum class eRootParam : uint8
	{
		PushConstants = 0, // b0, space0 — 8 DWORDs, per-draw, layout defined per pass.
		PassConstantBuffer = 1, // b1, space0 — optional, pass sets it when push constants aren't enough.
		FrameConstantBuffer = 2, // b2, space0 — all per frame data, set once per frame.
		Invalid
	};

	enum class eStaticSampler : uint32
	{
		LinearWrap = 0, // s0 — general surface textures.
		LinearClamp = 1, // s1 — post-processing, screen-space.
		PointClamp = 2,	// s2 — depth reads, ID buffers, exact texel fetch.
		AnisoWrap = 3, // s3 — high-quality surface textures.
		LinearBorder = 4, // s4 — shadow map out-of-bounds sampling.
		Count
	};

	// One common Root Signature.
	class RootSignature
	{
	public:
		static constexpr uint32 PushConstantCount = 8;

		RootSignature() = default;
		~RootSignature() = default;
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;
		RootSignature(RootSignature&&) noexcept = default;
		RootSignature& operator=(RootSignature&&) noexcept = default;

		void Create(GpuDevice& device);

		ID3D12RootSignature* Get() const { return m_pDxRootSignature.Get(); }

	private:
		static std::array<D3D12_STATIC_SAMPLER_DESC1, static_cast<size_t>(eStaticSampler::Count)> BuildStaticSamplers();

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pDxRootSignature = nullptr;
	};
}
