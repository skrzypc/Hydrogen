
#include <algorithm>
#include <DirectXMath.h>

#include "engine.h"
#include "config.h"
#include "logger.h"
#include "verifier.h"
#include "renderScene.h"
#include "modelLoader.h"
#include "primitiveBuilders.h"
#include "components/transformComponent.h"
#include "components/meshComponent.h"
#include "components/cameraComponent.h"
#include "components/lightComponent.h"
#include "hydrogenMath.h"
#include "ui/uiContext.h"

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

		//// Debug room: open-top box, no roof, for isolating path tracer bugs away from Sponza's complexity.
		//constexpr float32 RoomHalfSize = 2.0f;
		//constexpr float32 RoomHeight = 3.0f;
		//constexpr float32 RoomWallHalfThickness = 0.05f;
		//constexpr float32 RoomFloorHalfThickness = 0.05f;

		//const auto addBoxEntity = [this](DirectX::XMFLOAT3 halfExtents, DirectX::XMFLOAT3 position, std::string name)
		//{
		//	Mesh boxMesh = Primitives::BuildBox(halfExtents, name);
		//	MeshMetadata metaData{ .name = name };
		//	MeshHandle boxHandle = m_assetRegistry.RegisterMesh(std::move(metaData), std::move(boxMesh));

		//	Entity entity = m_scene.CreateEntity();
		//	Transform transform{};
		//	transform.position = position;
		//	m_scene.transforms.Add(entity, TransformComponent{ transform });
		//	m_scene.meshes.Add(entity, MeshComponent{ boxHandle });
		//};

		//addBoxEntity({ RoomHalfSize, RoomFloorHalfThickness, RoomHalfSize }, { 0.0f, -RoomFloorHalfThickness, 0.0f }, "Floor");
		//addBoxEntity({ RoomWallHalfThickness, RoomHeight * 0.5f, RoomHalfSize }, {  RoomHalfSize, RoomHeight * 0.5f, 0.0f }, "WallPosX");
		//addBoxEntity({ RoomWallHalfThickness, RoomHeight * 0.5f, RoomHalfSize }, { -RoomHalfSize, RoomHeight * 0.5f, 0.0f }, "WallNegX");
		//addBoxEntity({ RoomHalfSize, RoomHeight * 0.5f, RoomWallHalfThickness }, { 0.0f, RoomHeight * 0.5f,  RoomHalfSize }, "WallPosZ");
		//addBoxEntity({ RoomHalfSize, RoomHeight * 0.5f, RoomWallHalfThickness }, { 0.0f, RoomHeight * 0.5f, -RoomHalfSize }, "WallNegZ");

		//// Point lights
		//{
		//	const auto addPointLight = [this](DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float32 intensity)
		//	{
		//		Entity entity = m_scene.CreateEntity();
		//		Transform transform{};
		//		transform.position = position;
		//		m_scene.transforms.Add(entity, TransformComponent{ transform });
		//		m_scene.lights.Add(entity, LightComponent{ Light{ .type = eLightType::Point, .color = color, .intensity = intensity } });
		//	};

		//	addPointLight({  1.2f, RoomHeight - 0.5f,  1.2f }, { 1.0f, 0.0f, 0.0f }, 20.0f);
		//	addPointLight({ -1.2f, RoomHeight - 0.5f, -1.2f }, { 0.0f, 1.0f, 0.0f }, 20.0f);
		//}

		// Load model and populate scene
		{
			std::vector<Model> models{};
			//models.emplace_back(ModelLoader::Load("data/models/stanfordBunny/scene.gltf"));
			models.emplace_back(ModelLoader::Load("data/models/AmdSponza/MainSponza.gltf"));
			{
				//models.emplace_back(ModelLoader::Load("data/models/IntelSponza/main_sponza/NewSponza_Main_glTF_003.gltf"));
				//models.emplace_back(ModelLoader::Load("data/models/IntelSponza/pkg_a_curtains/NewSponza_Curtains_glTF.gltf"));
				//models.emplace_back(ModelLoader::Load("data/models/IntelSponza/pkg_b_ivy/NewSponza_IvyGrowth_glTF.gltf"));
				//models.emplace_back(ModelLoader::Load("data/models/IntelSponza/pkg_c_trees/NewSponza_CypressTree_glTF.gltf"));
			}
			//models.emplace_back(ModelLoader::Load("data/models/cornell_box/scene.gltf"));

			for (Model& model : models)
			{
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

					if (node.lightIndex.has_value())
					{
						//if (m_scene.lights.GetAll().size() == 0)
						{
							m_scene.lights.Add(entity, LightComponent{ model.lights[*node.lightIndex] });
						}
					}
				}
			}
		}

		// Camera
		{
			m_activeCamera = m_scene.CreateEntity();
			Transform cameraTransform{};
			cameraTransform.position = { 0.0f, 0.2f, -1.0f };
			m_scene.transforms.Add(m_activeCamera, TransformComponent{ cameraTransform });
			m_scene.cameras.Add(m_activeCamera, CameraComponent{});
		}

		int32 returnCode = 0;
		while (true)
		{
			if (const auto ecode = m_window.ProcessMessages())
			{
				returnCode = *ecode;
				break;
			}


			m_debugUi.BeginFrame();

			const float32 deltaTime = static_cast<float32>(m_frameTimer.GetSeconds());
			m_frameTimer.Mark();

			const float64 time = m_appTimer.GetSeconds();

			RenderScene renderScene{};

			// Update camera
			{
				constexpr float32 sensitivity = 0.1f;

				const bool wantsMouseCapture = m_debugUi.WantsMouseCapture();
				const bool wantsKeyboardCapture = m_debugUi.WantsKeyboardCapture();

				TransformComponent* pTransformComponent = m_scene.transforms.Get(m_activeCamera);

				if (pTransformComponent && !wantsMouseCapture && m_window.IsRightMouseDown())
				{
					const Vector3 currentEuler = Quaternion(pTransformComponent->transform.rotation).ToEuler();
					const float32 pitch = std::clamp(ToDegrees(currentEuler.x) + m_window.GetMouseDeltaY() * sensitivity, -89.0f, 89.0f);
					const float32 yaw = ToDegrees(currentEuler.y) + m_window.GetMouseDeltaX() * sensitivity;

					XMStoreFloat4(&pTransformComponent->transform.rotation,
						Quaternion::CreateFromYawPitchRoll(ToRadians(yaw), ToRadians(pitch), 0.0f));
				}

				const Quaternion orientation = pTransformComponent ? Quaternion(pTransformComponent->transform.rotation) : Quaternion::Identity;
				const Vector3 forward = Vector3::Transform(Forward, orientation);
				const Vector3 right = Vector3::Transform(Right, orientation);

				Vector3 move = Vector3::Zero;
				if (!wantsKeyboardCapture)
				{
					if (m_window.IsKeyDown('W')) { move += forward; }
					if (m_window.IsKeyDown('S')) { move -= forward; }
					if (m_window.IsKeyDown('D')) { move += right; }
					if (m_window.IsKeyDown('A')) { move -= right; }
					if (m_window.IsKeyDown('E')) { move += Up; }
					if (m_window.IsKeyDown('Q')) { move -= Up; }
				}

				if (move.LengthSquared() > 0.0f)
				{
					move.Normalize();
					move *= m_cameraSpeed * deltaTime;
				}

				if (pTransformComponent)
				{
					pTransformComponent->transform.position.x += move.x;
					pTransformComponent->transform.position.y += move.y;
					pTransformComponent->transform.position.z += move.z;

					renderScene.camera.position = pTransformComponent->transform.position;
					renderScene.camera.rotation = pTransformComponent->transform.rotation;
				}

				if (CameraComponent* pCameraComponent = m_scene.cameras.Get(m_activeCamera))
				{
					if (!wantsMouseCapture)
					{
						if (const float32 scroll = m_window.GetScrollDelta(); scroll != 0.0f)
						{
							if (m_window.IsRightMouseDown())
							{
								m_cameraSpeed = std::clamp(m_cameraSpeed + scroll * 0.5f, 0.5f, 20.0f);
							}
							else
							{
								pCameraComponent->fovYDeg = std::clamp(pCameraComponent->fovYDeg - scroll * 2.0f, 10.0f, 120.0f);
							}
						}

						if (m_window.IsMiddleMouseJustPressed())
						{
							pCameraComponent->fovYDeg = CameraComponent{}.fovYDeg;
						}
					}

					renderScene.camera.fovYDeg = pCameraComponent->fovYDeg;
					renderScene.camera.nearZ = pCameraComponent->nearZ;
					renderScene.camera.farZ = pCameraComponent->farZ;
					renderScene.camera.exposure = pCameraComponent->exposure;
				}
			}

			UiContext uiContext{};
			uiContext.pScene = &m_scene;
			uiContext.pAssetRegistry = &m_assetRegistry;
			uiContext.fnBuildRendererUi = [this]() { m_renderer.BuildBackendUI(); };
			uiContext.deltaTime = deltaTime;
			uiContext.time = time;

			m_debugUi.Draw(uiContext);

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

			const auto& lightEntities = m_scene.lights.GetEntities();
			auto lightComponents = m_scene.lights.GetAll();
			for (uint32 i = 0; i < static_cast<uint32>(lightEntities.size()); ++i)
			{
				const TransformComponent* tc = m_scene.transforms.Get(lightEntities[i]);
				if (!tc)
				{
					continue;
				}

				RenderLight renderLight{};
				renderLight.light = lightComponents[i].light;
				renderLight.position = tc->transform.position;
				renderLight.direction = Vector3::Transform(Forward, Quaternion(tc->transform.rotation));
				renderScene.lights.push_back(renderLight);
			}

			ImDrawData* drawData = m_debugUi.EndFrame();

			m_renderer.RenderFrame(renderScene, drawData, time, deltaTime);
		}

		return returnCode;
	}
}
