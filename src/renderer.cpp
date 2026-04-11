
#include <d3d12.h>
#include <DirectXColors.h>

#include <pix3.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

#include "frameGraph.h"
#include "pipelineState.h"

namespace Hydrogen
{
	void Renderer::Initialize(HWND hWnd)
	{
		m_gpuDevice.Create();
		m_swapChain.Create(m_gpuDevice, hWnd);
	}

	void Renderer::RenderFrame()
	{
		static bool bFirst = true;
		static uint64 fenceValues[Config::FramesInFlight] = { 0ull, 0ull, 0ull };
		static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators[Config::FramesInFlight];
		static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> commandLists[Config::FramesInFlight];

		static std::unordered_map<std::string, Shader> shaderCache{};
		static PipelineState testPso{};

		if (bFirst)
		{
			for (uint32 i = 0; i < Config::FramesInFlight; ++i)
			{
				H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])), "Failed to create command allocator!");
				H2_VERIFY_FATAL(m_gpuDevice.GetDxDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&commandLists[i])), "Failed to create command list!");
				commandLists[i]->SetName(String::Format(L"COMMAND_LIST_{}", i).c_str());
				commandLists[i]->ClearState(nullptr);
				commandLists[i]->Close();
				commandAllocators[i]->SetName(String::Format(L"COMMAND_ALLOCATOR_{}", i).c_str());
				commandAllocators[i]->Reset();
			}

			m_frameGraph.Initialize(m_gpuDevice);

			Shader::Desc testVsDesc
			{
				.sourcePath = "testShader.vs",
				.name = "TestVs",
				.entryPoint = "main",
				.type = eShaderType::VS,
				.defines = {}
			};

			Shader::Desc testPsDesc
			{
				.sourcePath = "testShader.ps",
				.name = "TestPs",
				.entryPoint = "main",
				.type = eShaderType::PS,
				.defines = {}
			};

			Shader testVs(testVsDesc);
			m_shaderCompiler.Compile(testVs);

			Shader testPs(testPsDesc);
			m_shaderCompiler.Compile(testPs);

			const DXGI_FORMAT backBufferFormat = m_swapChain.GetCurrentBackBuffer()->GetDesc().format;
			PipelineState::GraphicsDesc psoDesc
			{
				.pVertexShader = &testVs,
				.pPixelShader = &testPs,
				.renderTargetFormats = std::span<const DXGI_FORMAT>(&backBufferFormat, 1),
				.depthFormat = DXGI_FORMAT_UNKNOWN,
				.rasterizerDesc = PipelineState::DefaultRasterizer(),
				.blendDesc = PipelineState::DefaultBlend(),
				.depthStencilDesc = PipelineState::DefaultDepthStencil(),
				.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			};
			// Depth test not needed for fullscreen triangle.
			psoDesc.depthStencilDesc.DepthEnable = FALSE;

			testPso.CreateGraphics(m_gpuDevice, psoDesc);

			shaderCache.insert(std::make_pair("TestVs", std::move(testVs)));
			shaderCache.insert(std::make_pair("TestPs", std::move(testPs)));

			bFirst = false;
		}

		uint64 currentFrameNumber = m_swapChain.GetCurrentFrameNumber();
		uint32 currentFrameIndex = m_swapChain.GetCurrentFrameIndex();

		H2_INFO(eLogLevel::Minimal, "Current frame: {}", currentFrameNumber);

		auto& commandQueue = m_gpuDevice.GetDirectCommandQueue();
		commandQueue.Wait(fenceValues[currentFrameIndex]);

		commandAllocators[currentFrameIndex]->Reset();
		commandLists[currentFrameIndex]->Reset(commandAllocators[currentFrameIndex].Get(), nullptr);

		auto pCommandList = commandLists[currentFrameIndex].Get();

		// Bind descriptor heaps and root signature once per frame, before any pass executes.
		{
			std::vector<ID3D12DescriptorHeap*> descriptorHeaps(2, nullptr);
			descriptorHeaps[0] = m_gpuDevice.GetDescriptorHeap(eDescriptorHeapType::CBV_SRV_UAV).GetDxHeap();
			descriptorHeaps[1] = m_gpuDevice.GetDescriptorHeap(eDescriptorHeapType::Sampler).GetDxHeap();

			pCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());

			pCommandList->SetGraphicsRootSignature(m_gpuDevice.GetRootSignature().Get());
			//pCommandList->SetComputeRootSignature(pRS);
		}

		// Frame graph usage.
		{
			m_frameGraph.BeginFrame(currentFrameNumber);

			FGResourceHandle backBufferHandle = m_frameGraph.ImportTexture(m_swapChain.GetCurrentBackBuffer(), "Backbuffer");

			struct ClearPassData
			{
				FGResourceHandle backBufferHandle{};
			};

			m_frameGraph.AddPass<ClearPassData>(
				"ClearBackbuffer",
				[&](FGBuilder& frameGraphBuilder, ClearPassData& passData)
				{
					backBufferHandle = frameGraphBuilder.DefineOutput(backBufferHandle, FGAccess::Output::RenderTarget);
					passData.backBufferHandle = backBufferHandle;
				},
				[](const ClearPassData& passData, FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd)
				{
					PIXScopedEvent(cmd, PIX_COLOR_INDEX(3), "Clear target");
					constexpr FLOAT clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
					cmd->ClearRenderTargetView(ctx.GetRTV(passData.backBufferHandle), clearColor, 0, nullptr);
				}
			);

			const Texture::Desc& bbDesc = m_swapChain.GetCurrentBackBuffer()->GetDesc();

			struct DrawPassData
			{
				FGResourceHandle backBufferHandle{};
				ID3D12PipelineState* pPso = nullptr;
				uint32 width = 0;
				uint32 height = 0;
			};

			m_frameGraph.AddPass<DrawPassData>(
				"TestTriangle",
				[&](FGBuilder& frameGraphBuilder, DrawPassData& passData)
				{
					backBufferHandle = frameGraphBuilder.DefineOutput(backBufferHandle, FGAccess::Output::RenderTarget);
					passData.backBufferHandle = backBufferHandle;
					passData.pPso = testPso.Get();
					passData.width = bbDesc.width;
					passData.height = bbDesc.height;
				},
				[](const DrawPassData& passData, FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd)
				{
					PIXScopedEvent(cmd, PIX_COLOR_INDEX(5), "Test triangle");

					D3D12_VIEWPORT viewport{ 0.0f, 0.0f, static_cast<float32>(passData.width), static_cast<float32>(passData.height), 0.0f, 1.0f };
					D3D12_RECT scissor{ 0, 0, static_cast<uint32>(passData.width), static_cast<uint32>(passData.height) };
					cmd->RSSetViewports(1, &viewport);
					cmd->RSSetScissorRects(1, &scissor);

					D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.GetRTV(passData.backBufferHandle);
					cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
					cmd->SetPipelineState(passData.pPso);
					cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmd->DrawInstanced(3, 1, 0, 0);
				}
			);

			m_frameGraph.Compile();
			m_frameGraph.Execute(pCommandList);

			m_frameGraph.Reset();
		}

		pCommandList->Close();

		fenceValues[currentFrameIndex] = m_gpuDevice.GetDirectCommandQueue().SubmitCommandList(pCommandList);

		m_swapChain.Present();
	}
}
