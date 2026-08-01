
#include <algorithm>
#include <DirectXMath.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

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
			//Model model = ModelLoader::Load("data/models/stanfordBunny/scene.gltf");
			Model model = ModelLoader::Load("data/models/SponzaNew/MainSponza.gltf");

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

		// Camera
		{
			m_activeCamera = m_scene.CreateEntity();
			Transform cameraTransform{};
			cameraTransform.position = { 0.0f, 0.2f, -1.0f };
			m_scene.transforms.Add(m_activeCamera, TransformComponent{ cameraTransform });
			m_scene.cameras.Add(m_activeCamera, CameraComponent{});
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


			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			const float32 deltaTime = static_cast<float32>(m_frameTimer.GetSeconds());
			m_frameTimer.Mark();

			const float64 time = m_appTimer.GetSeconds();

			RenderScene renderScene{};

			// Update camera
			{
				constexpr float32 sensitivity = 0.1f;

				if (m_window.IsRightMouseDown())
				{
					m_yaw += m_window.GetMouseDeltaX() * sensitivity;
					m_pitch += m_window.GetMouseDeltaY() * sensitivity;
					m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
				}

				const Quaternion orientation = Quaternion::CreateFromYawPitchRoll(
					ToRadians(m_yaw),
					ToRadians(m_pitch),
					0.0f);

				const Vector3 forward = Vector3::Transform(Forward, orientation);
				const Vector3 right = Vector3::Transform(Right, orientation);

				Vector3 move = Vector3::Zero;
				if (m_window.IsKeyDown('W')) { move += forward; }
				if (m_window.IsKeyDown('S')) { move -= forward; }
				if (m_window.IsKeyDown('D')) { move += right; }
				if (m_window.IsKeyDown('A')) { move -= right; }
				if (m_window.IsKeyDown('E')) { move += Up; }
				if (m_window.IsKeyDown('Q')) { move -= Up; }

				if (move.LengthSquared() > 0.0f)
				{
					move.Normalize();
					move *= m_cameraSpeed * deltaTime;
				}

				if (TransformComponent* tc = m_scene.transforms.Get(m_activeCamera))
				{
					tc->transform.position.x += move.x;
					tc->transform.position.y += move.y;
					tc->transform.position.z += move.z;
					XMStoreFloat4(&tc->transform.rotation, orientation);

					renderScene.camera.position = tc->transform.position;
					renderScene.camera.rotation = tc->transform.rotation;
				}

				if (CameraComponent* cc = m_scene.cameras.Get(m_activeCamera))
				{
					if (const float32 scroll = m_window.GetScrollDelta(); scroll != 0.0f)
					{
						if (m_window.IsRightMouseDown())
						{
							m_cameraSpeed = std::clamp(m_cameraSpeed + scroll * 0.5f, 0.5f, 20.0f);
						}
						else
						{
							cc->fovYDeg = std::clamp(cc->fovYDeg - scroll * 2.0f, 10.0f, 120.0f);
						}
					}

					if (m_window.IsMiddleMouseJustPressed())
					{
						cc->fovYDeg = CameraComponent{}.fovYDeg;
					}

					renderScene.camera.fovYDeg = cc->fovYDeg;
					renderScene.camera.nearZ = cc->nearZ;
					renderScene.camera.farZ = cc->farZ;
				}
			}

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

			{
				ImGui::Begin("Debug");
				ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
				if (const TransformComponent* tc = m_scene.transforms.Get(m_activeCamera))
				{
					ImGui::Text("Pos: %.2f %.2f %.2f", tc->transform.position.x, tc->transform.position.y, tc->transform.position.z);
					ImGui::Text("Yaw: %.1f  Pitch: %.1f", m_yaw, m_pitch);
				}
				ImGui::Text("Speed: %.2f", m_cameraSpeed);
				if (const CameraComponent* cc = m_scene.cameras.Get(m_activeCamera))
				{
					ImGui::Text("FOV: %.1f", cc->fovYDeg);
				}
				ImGui::End();
			}

			m_renderer.BuildBackendUI();

			ImGui::Render();
			ImDrawData* drawData = ImGui::GetDrawData();

			m_renderer.RenderFrame(renderScene, drawData, time, deltaTime);
		}

		return returnCode;
	}
}
