
#include "gpuScene.h"

#include <DirectXMath.h>

#include "device.h"
#include "gpuUploader.h"
#include "graphicsContext.h"
#include "mesh.h"
#include "stringUtilities.h"
#include "verifier.h"

namespace Hydrogen
{
	void GpuScene::Initialize(GpuDevice& device, GpuUploader& uploader, uint32 maxVertices, uint32 maxIndices, uint32 maxObjects, uint32 maxViews)
	{
		m_pDevice = &device;
		m_pUploader = &uploader;
		m_maxVertices = maxVertices;
		m_maxIndices = maxIndices;
		m_maxObjects = maxObjects;

		ResourceState initialState{};

		m_positionBuffer = device.CreateBuffer(L"H2_SCENE_POSITIONS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) }, initialState);
		m_normalBuffer = device.CreateBuffer(L"H2_SCENE_NORMALS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT3) }, initialState);
		m_uvBuffer = device.CreateBuffer(L"H2_SCENE_UVS", Buffer::Desc{ .size = maxVertices * sizeof(DirectX::XMFLOAT2) }, initialState);
		m_indexBuffer = device.CreateBuffer(L"H2_SCENE_INDICES", Buffer::Desc{ .size = maxIndices * sizeof(uint32) }, initialState);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		srvDesc.Buffer.NumElements = maxVertices;
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT3);
		m_positionSrv = device.CreateShaderResourceView(m_positionBuffer.get(), srvDesc);
		m_normalSrv = device.CreateShaderResourceView(m_normalBuffer.get(), srvDesc);

		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT2);
		m_uvSrv = device.CreateShaderResourceView(m_uvBuffer.get(), srvDesc);

		srvDesc.Buffer.NumElements = maxIndices;
		srvDesc.Buffer.StructureByteStride = sizeof(uint32);
		m_indexSrv = device.CreateShaderResourceView(m_indexBuffer.get(), srvDesc);

		m_transformBuffer = device.CreateUploadBuffer(L"H2_SCENE_TRANSFORMS", maxObjects * sizeof(DirectX::XMFLOAT4X4));
		srvDesc.Buffer.NumElements = maxObjects;
		srvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);
		m_transformBufferSrv = device.CreateShaderResourceView(m_transformBuffer.get(), srvDesc);

		for (uint32 i = 0; i < Config::FramesInFlight; ++i)
		{
			m_instanceDescs[i] = device.CreateUploadBuffer(L"H2_SCENE_TLAS_INSTANCES", maxObjects * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
		}
	}

	void GpuScene::RegisterMesh(MeshHandle handle, const Mesh& mesh)
	{
		m_meshUploadQueue.push(QueuedMesh{ handle, mesh });
	}

	void GpuScene::RegisterMeshes(std::vector<MeshHandle>& meshHandles, std::vector<Mesh>& meshes)
	{
		for (uint32 i = 0; i < meshHandles.size(); ++i)
		{
			m_meshUploadQueue.push(QueuedMesh{ meshHandles[i], std::move(meshes[i]) });
		}
	}

	SceneBindings GpuScene::Update(const RenderScene& renderScene, uint32 frameIndex)
	{
		m_currentFrameIndex = frameIndex;

		DrainUploadQueue();
		PromoteCompletedUploads();
		PromoteCompletedBLAS();

		UpdateTransforms(renderScene.objects);

		SceneBindings bindings{};
		bindings.positionBufferIndex = m_positionSrv.index;
		bindings.normalBufferIndex = m_normalSrv.index;
		bindings.uvBufferIndex = m_uvSrv.index;
		bindings.indexBufferIndex = m_indexSrv.index;
		bindings.transformBufferIndex = m_transformBufferSrv.index;
		return bindings;
	}

	void GpuScene::DrainUploadQueue()
	{
		if (m_meshUploadQueue.empty())
		{
			return;
		}

		const uint32 firstPendingIdx = static_cast<uint32>(m_pendingUploads.size());
		uint32 count = 0;

		while (!m_meshUploadQueue.empty() && count < m_maxMeshUploadsPerFrame)
		{
			QueuedMesh& queued = m_meshUploadQueue.front();
			const uint32 meshIndex = static_cast<uint32>(m_meshes.size());

			StageMesh(queued.handle, queued.mesh);
			m_pendingUploads.push_back(PendingMeshUpload{ meshIndex, queued.handle.id, 0 });
			m_meshUploadQueue.pop();
			++count;
		}

		const uint64 fence = m_pUploader->Flush();
		for (uint32 i = firstPendingIdx; i < static_cast<uint32>(m_pendingUploads.size()); ++i)
		{
			m_pendingUploads[i].copyFence = fence;
		}
	}

	void GpuScene::PromoteCompletedUploads()
	{
		if (m_pendingUploads.empty())
		{
			return;
		}

		const uint64 completedCopy = m_pDevice->GetCompletedFenceValue<eQueueType::Copy>();

		std::vector<PendingMeshUpload> ready{};
		std::vector<PendingMeshUpload> remaining{};
		for (auto& entry : m_pendingUploads)
		{
			if (entry.copyFence <= completedCopy)
			{
				ready.push_back(entry);
			}
			else
			{
				remaining.push_back(entry);
			}
		}
		m_pendingUploads = std::move(remaining);

		if (!ready.empty())
		{
			BuildPendingBlas(ready);
		}
	}

	void GpuScene::BuildPendingBlas(std::span<const PendingMeshUpload> uploads)
	{
		uint64 totalScratchSize = 0ull;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> rtGeometryDescs(uploads.size());
		std::vector<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO> blasPrebuildInfos(uploads.size());
		std::vector<uint64> scratchOffsets(uploads.size());

		for (uint32 i = 0; i < uploads.size(); ++i)
		{
			const GpuMesh& mesh = m_meshes[uploads[i].meshIndex];

			rtGeometryDescs[i] = {};
			rtGeometryDescs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			rtGeometryDescs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			rtGeometryDescs[i].Triangles.VertexBuffer.StartAddress = m_positionBuffer->GetResource()->GetGPUVirtualAddress() + mesh.baseVertex * sizeof(DirectX::XMFLOAT3);
			rtGeometryDescs[i].Triangles.VertexBuffer.StrideInBytes = sizeof(DirectX::XMFLOAT3);
			rtGeometryDescs[i].Triangles.VertexCount = mesh.vertexCount;
			rtGeometryDescs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			rtGeometryDescs[i].Triangles.IndexBuffer = m_indexBuffer->GetResource()->GetGPUVirtualAddress() + mesh.baseIndex * sizeof(uint32);
			rtGeometryDescs[i].Triangles.IndexCount = mesh.indexCount;
			rtGeometryDescs[i].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &rtGeometryDescs[i];

			m_pDevice->GetDxDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &blasPrebuildInfos[i]);

			scratchOffsets[i] = totalScratchSize;
			const uint64 alignedScratch = (blasPrebuildInfos[i].ScratchDataSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1)
				& ~static_cast<uint64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1);
			totalScratchSize += alignedScratch;
		}

		auto& scratch = m_blasScratch[m_currentFrameIndex];
		if (!scratch || scratch->GetDesc().size < totalScratchSize)
		{
			ResourceState uavState{};
			scratch = m_pDevice->CreateBuffer(L"H2_BLAS_SCRATCH",
				Buffer::Desc{ .size = totalScratchSize, .flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS },
				uavState);
		}
		const D3D12_GPU_VIRTUAL_ADDRESS scratchBase = scratch->GetResource()->GetGPUVirtualAddress();

		GraphicsContext graphicsContext = m_pDevice->AcquireGraphicsContext();
		const uint32 firstBlasIdx = static_cast<uint32>(m_blasBuffers.size());

		std::vector<D3D12_BUFFER_BARRIER> blasBarriers;
		blasBarriers.reserve(uploads.size());

		for (uint32 i = 0; i < uploads.size(); ++i)
		{
			const GpuMesh& mesh = m_meshes[uploads[i].meshIndex];

			ResourceState asState{};
			asState.access = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;

			m_blasBuffers.push_back(m_pDevice->CreateBuffer(
				String::ToWide(String::Format("H2_BLAS_{}", String::ToUpper(mesh.name))),
				Buffer::Desc{
					.size = blasPrebuildInfos[i].ResultDataMaxSizeInBytes,
					.flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE },
					asState));

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &rtGeometryDescs[i];

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
			buildDesc.Inputs = inputs;
			buildDesc.DestAccelerationStructureData = m_blasBuffers.back()->GetResource()->GetGPUVirtualAddress();
			buildDesc.ScratchAccelerationStructureData = scratchBase + scratchOffsets[i];

			graphicsContext.CmdList()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

			blasBarriers.push_back(D3D12_BUFFER_BARRIER
				{
					.SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
					.SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE,
					.AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE,
					.AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,
					.pResource = m_blasBuffers.back()->GetResource(),
					.Offset = 0,
					.Size = UINT64_MAX,
				});

			m_pendingBlasBuilds.push_back(PendingBlas{ uploads[i].meshIndex, uploads[i].handleId, firstBlasIdx + i, 0 });
		}

		D3D12_BARRIER_GROUP blasBarrierGroup
		{
			.Type = D3D12_BARRIER_TYPE_BUFFER,
			.NumBarriers = static_cast<uint32>(blasBarriers.size()),
			.pBufferBarriers = blasBarriers.data(),
		};
		graphicsContext.CmdList()->Barrier(1, &blasBarrierGroup);

		const uint64 fence = m_pDevice->ExecuteGraphicsContext(std::move(graphicsContext));
		for (auto& entry : m_pendingBlasBuilds)
		{
			if (entry.directFence == 0)
			{
				entry.directFence = fence;
			}
		}
	}

	void GpuScene::PromoteCompletedBLAS()
	{
		if (m_pendingBlasBuilds.empty())
		{
			return;
		}

		const uint64 completedDirect = m_pDevice->GetCompletedFenceValue<eQueueType::Direct>();

		std::vector<PendingBlas> remaining;
		for (auto& entry : m_pendingBlasBuilds)
		{
			if (entry.directFence <= completedDirect)
			{
				const uint64 address = m_blasBuffers[entry.blasBufferIndex]->GetResource()->GetGPUVirtualAddress();
				m_meshes[entry.meshIndex].blasAddress = address;
				m_gpuMeshCache[entry.handleId].blasAddress = address;
			}
			else
			{
				remaining.push_back(entry);
			}
		}
		m_pendingBlasBuilds = std::move(remaining);
	}

	void GpuScene::UpdateTransforms(std::span<const RenderObject> objects)
	{
		auto* pDescs = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(
			m_instanceDescs[m_currentFrameIndex]->GetMappedPtr());

		uint32 instanceCount = 0;
		for (uint32 transformIndex = 0; transformIndex < static_cast<uint32>(objects.size()); ++transformIndex)
		{
			const RenderObject& obj = objects[transformIndex];

			m_transformBuffer->Write(&obj.worldMatrix, sizeof(DirectX::XMFLOAT4X4), transformIndex * sizeof(DirectX::XMFLOAT4X4));

			const GpuMesh* pMesh = GetGpuMesh(obj.mesh);
			D3D12_RAYTRACING_INSTANCE_DESC& desc = pDescs[transformIndex];
			desc = {};
			desc.InstanceMask = (pMesh && pMesh->blasAddress != 0) ? 0xFF : 0x00;
			if (pMesh && pMesh->blasAddress != 0)
			{
				DirectX::XMMATRIX worldMatrix = DirectX::XMLoadFloat4x4(&obj.worldMatrix);
				DirectX::XMMATRIX worldMatrixT = DirectX::XMMatrixTranspose(worldMatrix);
				memcpy(desc.Transform, &worldMatrixT, sizeof(desc.Transform));
				desc.InstanceID = transformIndex;
				desc.AccelerationStructure = pMesh->blasAddress;
				++instanceCount;
			}
		}

		m_lastInstanceCount = instanceCount;
	}

	void GpuScene::StageMesh(MeshHandle handle, const Mesh& src)
	{
		const uint32 vertexCount = static_cast<uint32>(src.positions.size());
		const uint32 indexCount = static_cast<uint32>(src.indices.size());

		H2_VERIFY_FATAL(m_nextVertex + vertexCount <= m_maxVertices, "GpuScene vertex capacity exceeded!");
		H2_VERIFY_FATAL(m_nextIndex + indexCount <= m_maxIndices, "GpuScene index capacity exceeded!");

		m_pUploader->Upload(src.positions.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_positionBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.normals.data(), vertexCount * sizeof(DirectX::XMFLOAT3), m_normalBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT3));
		m_pUploader->Upload(src.uvs.data(), vertexCount * sizeof(DirectX::XMFLOAT2), m_uvBuffer.get(), m_nextVertex * sizeof(DirectX::XMFLOAT2));
		m_pUploader->Upload(src.indices.data(), indexCount * sizeof(uint32), m_indexBuffer.get(), m_nextIndex * sizeof(uint32));

		GpuMesh& gpuMesh = m_meshes.emplace_back();
		gpuMesh.name = src.name;
		gpuMesh.baseVertex = m_nextVertex;
		gpuMesh.vertexCount = vertexCount;
		gpuMesh.baseIndex = m_nextIndex;
		gpuMesh.indexCount = indexCount;

		m_nextVertex += vertexCount;
		m_nextIndex += indexCount;

		if (handle.id >= m_gpuMeshCache.size())
		{
			m_gpuMeshCache.resize(handle.id + 1);
		}
		m_gpuMeshCache[handle.id] = gpuMesh;
	}

	const GpuMesh* GpuScene::GetGpuMesh(MeshHandle handle) const
	{
		if (handle.id >= m_gpuMeshCache.size()) { return nullptr; }
		return &m_gpuMeshCache[handle.id];
	}
}
