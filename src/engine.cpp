
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

		// Load model and populate scene
		{
			//Model model = ModelLoader::Load("data/models/stanfordBunny/scene.gltf");
			Model model = ModelLoader::Load("data/models/SponzaNew/MainSponza.gltf");
			//Model model = ModelLoader::Load("data/models/main_sponza/NewSponza_Main_glTF_003.gltf");

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
					m_scene.lights.Add(entity, LightComponent{ model.lights[*node.lightIndex] });
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
