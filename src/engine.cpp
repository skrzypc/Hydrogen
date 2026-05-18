
#include "engine.h"
#include "config.h"
#include "logger.h"
#include "verifier.h"
#include "renderScene.h"
#include "modelLoader.h"
#include "primitiveBuilders.h"
#include "components/transformComponent.h"
#include "components/meshComponent.h"

namespace Hydrogen
{
	int32 Engine::Run(LPSTR commandLineArgs)
	{
		Logger::Initialize();

		if (Hydrogen::Config::WaitForDebugger)
		{
			while (!::IsDebuggerPresent())
			{
				::Sleep(1000);

				H2_INFO(eLogLevel::Verbose, "Waiting for debugger.");
			}
		}

		m_window.Create(Config::WindowWidth, Config::WindowHeight, L"Hydrogen Engine");

		m_renderer.Initialize(m_window.GetHandle());
		m_renderer.SetUploadQueue(&m_assetRegistry.GetUploadQueue());

		// Load model and populate scene
		{
			Model model = ModelLoader::Load("data/models/stanfordBunny/scene.gltf");

			std::vector<MeshHandle> handles;
			for (Mesh& mesh : model.meshes)
			{
				MeshMetadata metaData
				{
					.name = mesh.name
				};

				handles.push_back(m_assetRegistry.RegisterMesh(std::move(metaData), std::move(mesh)));
			}

			for (uint32 i = 0; i < static_cast<uint32>(model.nodes.size()); ++i)
			{
				const ModelNode& node = model.nodes[i];

				Entity entity = m_scene.CreateEntity();
				m_scene.transforms.Add(entity, TransformComponent{ node.localTransform });
				if (node.meshIndex.has_value())
				{
					m_scene.meshes.Add(entity, MeshComponent{ handles[*node.meshIndex] });
				}
			}
		}

		// Floor
		{
			Mesh floorMesh = Primitives::BuildBox({ 0.5f, 0.005f, 0.5f }, "Floor");
			MeshHandle floorHandle = m_assetRegistry.RegisterMesh({ .name = "Floor" }, std::move(floorMesh));

			Entity floor = m_scene.CreateEntity();
			Transform floorTransform{};
			floorTransform.position = { 0.0f, -0.1f, 0.0f };
			m_scene.transforms.Add(floor, TransformComponent{ floorTransform });
			m_scene.meshes.Add(floor, MeshComponent{ floorHandle });
		}

		int32 returnCode = 0;
		while (true)
		{
			if (const auto ecode = m_window.ProcessMessages())
			{
				returnCode = *ecode;
				break;
			}


			// Build RenderScene from ECS Scene
			RenderScene renderScene{};
			const auto& meshEntities = m_scene.meshes.GetEntities();
			auto meshComponents = m_scene.meshes.GetAll();
			for (uint32 i = 0; i < static_cast<uint32>(meshEntities.size()); ++i)
			{
				const TransformComponent* tc = m_scene.transforms.Get(meshEntities[i]);
				if (!tc)
				{
					continue;
				}

				RenderObject obj{};
				obj.mesh = meshComponents[i].mesh;
				DirectX::XMStoreFloat4x4(&obj.worldMatrix, tc->transform.GetWorldMatrix());
				renderScene.objects.push_back(obj);
			}

			m_renderer.RenderFrame(renderScene);
		}

		return returnCode;
	}
}
