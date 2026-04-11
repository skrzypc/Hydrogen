#include "pipelineState.h"
#include "device.h"
#include "verifier.h"

namespace Hydrogen
{
	template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename T>
	struct alignas(void*) PSOSubObject
	{
		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = Type;
		T data = {};
	};

	struct GraphicsPipelineStream
	{
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> rootSignature;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE> vs;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE> ps;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2> rasterizer;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC> blend;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1> depthStencil;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY> rtFormats;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT> dsvFormat;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE> primitiveTopology;
	};

	struct ComputePipelineStream
	{
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*> rootSignature;
		PSOSubObject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE> cs;
	};

	void PipelineState::CreateGraphics(GpuDevice& device, const GraphicsDesc& desc)
	{
		H2_VERIFY_FATAL(desc.pVertexShader && desc.pVertexShader->IsCompiled(), "Vertex shader is not compiled!");
		H2_VERIFY_FATAL(desc.pVertexShader->GetDesc().type == eShaderType::VS, "Expected a vertex shader!");
		H2_VERIFY_FATAL(!desc.pPixelShader || desc.pPixelShader->GetDesc().type == eShaderType::PS, "Expected a pixel shader!");

		D3D12_RT_FORMAT_ARRAY rtFormatArray = {};
		rtFormatArray.NumRenderTargets = static_cast<uint32>(desc.renderTargetFormats.size());
		for (uint32 i = 0; i < rtFormatArray.NumRenderTargets; ++i)
		{
			rtFormatArray.RTFormats[i] = desc.renderTargetFormats[i];
		}

		GraphicsPipelineStream stream{};
		stream.rootSignature.data = device.GetRootSignature().Get();
		stream.vs.data = { desc.pVertexShader->GetBytecode(), desc.pVertexShader->GetBytecodeSize() };
		stream.ps.data = desc.pPixelShader ? D3D12_SHADER_BYTECODE{ desc.pPixelShader->GetBytecode(), desc.pPixelShader->GetBytecodeSize() } : D3D12_SHADER_BYTECODE{};
		stream.rasterizer.data = desc.rasterizerDesc;
		stream.blend.data = desc.blendDesc;
		stream.depthStencil.data = desc.depthStencilDesc;
		stream.rtFormats.data = rtFormatArray;
		stream.dsvFormat.data = desc.depthFormat;
		stream.primitiveTopology.data = desc.primitiveTopologyType;

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
		{
			.SizeInBytes = sizeof(GraphicsPipelineStream),
			.pPipelineStateSubobjectStream = &stream,
		};

		H2_VERIFY_FATAL(device.GetDxDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pDxPipelineState)), "Graphics PSO creation failed!");
	}

	void PipelineState::CreateCompute(GpuDevice& device, const ComputeDesc& desc)
	{
		H2_VERIFY_FATAL(desc.pComputeShader && desc.pComputeShader->IsCompiled(), "Compute shader is not compiled!");
		H2_VERIFY_FATAL(desc.pComputeShader->GetDesc().type == eShaderType::CS, "Expected a compute shader!");

		ComputePipelineStream stream;
		stream.rootSignature.data = device.GetRootSignature().Get();
		stream.cs.data = { desc.pComputeShader->GetBytecode(), desc.pComputeShader->GetBytecodeSize() };

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
		{
			.SizeInBytes = sizeof(ComputePipelineStream),
			.pPipelineStateSubobjectStream = &stream,
		};

		H2_VERIFY_FATAL(device.GetDxDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pDxPipelineState)), "Compute PSO creation failed!");
	}

	D3D12_RASTERIZER_DESC2 PipelineState::DefaultRasterizer()
	{
		return
		{
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK,
			.FrontCounterClockwise = FALSE,
			.DepthBias = 0,
			.DepthBiasClamp = 0.0f,
			.SlopeScaledDepthBias = 0.0f,
			.DepthClipEnable = TRUE,
			.LineRasterizationMode = D3D12_LINE_RASTERIZATION_MODE_ALIASED,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};
	}

	D3D12_BLEND_DESC PipelineState::DefaultBlend()
	{
		D3D12_BLEND_DESC desc = {};
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;
		desc.RenderTarget[0] =
		{
			.BlendEnable = FALSE,
			.LogicOpEnable = FALSE,
			.SrcBlend = D3D12_BLEND_ONE,
			.DestBlend = D3D12_BLEND_ZERO,
			.BlendOp = D3D12_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D12_BLEND_ONE,
			.DestBlendAlpha = D3D12_BLEND_ZERO,
			.BlendOpAlpha = D3D12_BLEND_OP_ADD,
			.LogicOp = D3D12_LOGIC_OP_NOOP,
			.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		return desc;
	}

	D3D12_DEPTH_STENCIL_DESC1 PipelineState::DefaultDepthStencil()
	{
		return
		{
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
			.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS },
			.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS },
			.DepthBoundsTestEnable = FALSE,
		};
	}
}
