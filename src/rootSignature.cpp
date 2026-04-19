
#include "rootSignature.h"
#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	void RootSignature::Create(GpuDevice& device)
	{
		constexpr uint32 paramCount = static_cast<uint32>(eRootParam::Invalid);
		D3D12_ROOT_PARAMETER1 params[paramCount]{};

		params[static_cast<uint32>(eRootParam::PushConstants)] =
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants =
			{
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.Num32BitValues = PushConstantCount,
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};

		params[static_cast<uint32>(eRootParam::PassConstantBuffer)] =
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
			.Descriptor =
			{
				.ShaderRegister = 1,
				.RegisterSpace = 0,
				.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, // Stable between binding and the draw/dispatch.
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};

		params[static_cast<uint32>(eRootParam::FrameConstantBuffer)] =
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
			.Descriptor =
			{
				.ShaderRegister = 2,
				.RegisterSpace = 0,
				.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, // Guaranteed not to change for the lifetime of the command list.
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};

		auto staticSamplers = BuildStaticSamplers();

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc =
		{
			.Version = D3D_ROOT_SIGNATURE_VERSION_1_2,
			.Desc_1_2 =
			{
				.NumParameters = paramCount,
				.pParameters = params,
				.NumStaticSamplers = static_cast<uint32>(staticSamplers.size()),
				.pStaticSamplers = staticSamplers.data(),
				.Flags =
					D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |	// enables ResourceDescriptorHeap[i] in HLSL
					D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |	// enables SamplerDescriptorHeap[i] in HLSL
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
			},
		};

		Microsoft::WRL::ComPtr<ID3DBlob> pBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

		H2_VERIFY_FATAL(D3D12SerializeVersionedRootSignature(&rsDesc, &pBlob, &pErrorBlob), "Root signature serialization failed: {}", pErrorBlob ? static_cast<const char*>(pErrorBlob->GetBufferPointer()) : "Unknown error.");

		H2_VERIFY_FATAL(device.GetDxDevice()->CreateRootSignature(
			0,
			pBlob->GetBufferPointer(),
			pBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_pDxRootSignature)),
			"Root signature creation failed"
		);

		m_pDxRootSignature->SetName(L"H2_COMMON_ROOT_SIGNATURE");
	}

	std::array<D3D12_STATIC_SAMPLER_DESC1, static_cast<size_t>(eStaticSampler::Count)> RootSignature::BuildStaticSamplers()
	{
		std::array<D3D12_STATIC_SAMPLER_DESC1, static_cast<size_t>(eStaticSampler::Count)> staticSamplers = {};

		D3D12_STATIC_SAMPLER_DESC1 base =
		{
			.MipLODBias = 0.0f,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0.0f,
			.MaxLOD = D3D12_FLOAT32_MAX,
			.RegisterSpace = 0,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			.Flags = D3D12_SAMPLER_FLAG_NONE,
		};

		// s0 — linear wrap.
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)] = base;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)].ShaderRegister = static_cast<UINT>(eStaticSampler::LinearWrap);

		// s1 — linear clamp.
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)] = staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)];
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)].ShaderRegister = static_cast<UINT>(eStaticSampler::LinearClamp);

		// s2 — point clamp (depth reads, ID buffers, exact texel fetch).
		staticSamplers[static_cast<uint32>(eStaticSampler::PointClamp)] = staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)];
		staticSamplers[static_cast<uint32>(eStaticSampler::PointClamp)].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSamplers[static_cast<uint32>(eStaticSampler::PointClamp)].ShaderRegister = static_cast<UINT>(eStaticSampler::PointClamp);

		// s3 — anisotropic 16x wrap (high-quality surface textures).
		staticSamplers[static_cast<uint32>(eStaticSampler::AnisoWrap)] = staticSamplers[static_cast<uint32>(eStaticSampler::LinearWrap)];
		staticSamplers[static_cast<uint32>(eStaticSampler::AnisoWrap)].Filter = D3D12_FILTER_ANISOTROPIC;
		staticSamplers[static_cast<uint32>(eStaticSampler::AnisoWrap)].MaxAnisotropy = 16;
		staticSamplers[static_cast<uint32>(eStaticSampler::AnisoWrap)].ShaderRegister = static_cast<UINT>(eStaticSampler::AnisoWrap);

		// s4 — linear border, opaque black (shadow map out-of-bounds = fully lit), no mip interpolation on shadow maps.
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)] = staticSamplers[static_cast<uint32>(eStaticSampler::LinearClamp)];
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		staticSamplers[static_cast<uint32>(eStaticSampler::LinearBorder)].ShaderRegister = static_cast<UINT>(eStaticSampler::LinearBorder);

		return staticSamplers;
	}
}
