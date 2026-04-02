
#include <d3d12.h>
#include <DirectXColors.h>

#include <pix3.h>

#include "renderer.h"
#include "logger.h"
#include "verifier.h"
#include "stringUtilities.h"

#include "frameGraph.h"

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

		// Frame graph usage.
		{
			m_frameGraph.BeginFrame(currentFrameNumber);

			FGResourceHandle backBufferHandle = m_frameGraph.ImportTexture(m_swapChain.GetCurrentBackBuffer(), "Backbuffer");

			const FLOAT* clearColor = nullptr;

			switch (currentFrameIndex)
			{
			case 0:
				clearColor = DirectX::Colors::CornflowerBlue;
				break;
			case 1:
				clearColor = DirectX::Colors::DarkRed;
				break;
			case 2:
				clearColor = DirectX::Colors::DarkGreen;
				break;
			default:
				clearColor = DirectX::Colors::White;
				break;
			}

			struct ClearPassData
			{
				FGResourceHandle backBufferHandle{};
				const FLOAT* clearColor = nullptr;
			};

			m_frameGraph.AddPass<ClearPassData>(
				"ClearBackbuffer",
				[&](FGBuilder& frameGraphBuilder, ClearPassData& passData)
				{
					backBufferHandle = frameGraphBuilder.DefineOutput(backBufferHandle, FGAccess::Output::RenderTarget);

					passData.backBufferHandle = backBufferHandle;
					passData.clearColor = clearColor;
				},
				[](const ClearPassData& passData, FGExecuteContext& ctx, ID3D12GraphicsCommandList7* cmd)
				{
					PIXScopedEvent(cmd, PIX_COLOR_INDEX(3), "Clear target");
					cmd->ClearRenderTargetView(ctx.GetRTV(passData.backBufferHandle), passData.clearColor, 0, nullptr);
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
