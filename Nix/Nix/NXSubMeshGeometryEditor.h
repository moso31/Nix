#pragma once
#include <string>
#include "BaseDefs/DX12.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

struct NXMeshData
{
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
	UINT pageIndex;
	UINT pageByteOffset;
};

struct NXMeshViews
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	D3D12_INDEX_BUFFER_VIEW ibv;
	UINT vertexCount;
	UINT indexCount;
};

enum NXPlaneAxis
{
	POSITIVE_X,
	POSITIVE_Y,
	POSITIVE_Z,
	NEGATIVE_X,
	NEGATIVE_Y,
	NEGATIVE_Z
};

class NXPrefab;
class NXPrimitive;
class NXSubMeshGeometryEditor : public NXInstance<NXSubMeshGeometryEditor>
{
public:
	NXSubMeshGeometryEditor();
	virtual ~NXSubMeshGeometryEditor();

	void Init(ID3D12Device* pDevice);

	void CreateFBXPrefab(NXPrefab* pPrefab, const std::string& filePath, bool bAutoCalcTangents);

	void CreateBox(NXPrimitive* pMesh, float x = 1.0f, float y = 1.0f, float z = 1.0f);
	void CreateCylinder(NXPrimitive* pMesh, float radius = 1.0f, float length = 3.0f, int segmentCircle = 16, int segmentLength = 4);
	void CreatePlane(NXPrimitive* pMesh, float width = 0.5f, float height = 0.5f, NXPlaneAxis Axis = POSITIVE_Y);
	void CreateSphere(NXPrimitive* pMesh, float radius = 1.0f, int segmentHorizontal = 16, int segmentVertical = 16);
	void CreateSHSphere(NXPrimitive* pMesh, int basis_l, int basis_m, float radius = 1.0f, int segmentHorizontal = 64, int segmentVertical = 64);

	// Editor objects
	void CreateMoveArrows(NXPrimitive* pMesh);

	template<class TVertex>
	void CreateVBIB(const std::vector<TVertex>& vertices, const std::vector<UINT>& indices, const std::string& name, bool forceCreate = false)
	{
		if (m_data.find(name) != m_data.end())
		{
			if (!forceCreate)
			{
				//throw std::exception("Mesh name already exists.");
				return;
			}
		}

		NXStructuredBufferArray<TVertex> pVB(vertices.size());
		NXStructuredBufferArray<UINT> pIB(indices.size());

		std::thread([&]() {
			pVB.WaitCreateComplete();
			pIB.WaitCreateComplete();

			NXMeshViews views;
			views.vbv.BufferLocation = pVB.CurrentGPUAddress();
			views.vbv.SizeInBytes = vbDataSize;
			views.vbv.StrideInBytes = sizeof(TVertex);
			views.ibv.BufferLocation = pIB.CurrentGPUAddress();
			views.ibv.SizeInBytes = ibDataSize;
			views.ibv.Format = DXGI_FORMAT_R32_UINT;
			views.indexCount = (UINT)indices.size();
			views.vertexCount = (UINT)vertices.size();

			m_mutex.lock();
			m_data[name] = std::move(views);
			m_mutex.unlock();

			UINT vbSize = (UINT)vertices.size() * sizeof(TVertex);
			UINT ibSize = (UINT)indices.size() * sizeof(UINT);

			UploadTaskContext vbTaskContext;
			UploadTaskContext ibTaskContext;

			if (NXUploadSystem->BuildTask(vbSize, vbTaskContext))
			{
				uint8_t* pDst = vbTaskContext.pResourceData + vbTaskContext.pResourceOffset;
				memcpy(pDst, vertices.data(), vbDataSize);
				NXUploadSystem->FinishTask(vbTaskContext);
			}
			else
			{
				throw std::exception("Failed to build upload task for vertex buffer.");
			}

			if (NXUploadSystem->BuildTask(ibSize, ibTaskContext))
			{
				uint8_t* pDst = ibTaskContext.pResourceData + ibTaskContext.pResourceOffset;
				memcpy(pDst, indices.data(), ibDataSize);
				NXUploadSystem->FinishTask(ibTaskContext);
			}
			else
			{
				throw std::exception("Failed to build upload task for index buffer.");
			}

			}).detach();
	}

	const NXMeshViews& GetMeshViews(const std::string& name);

	void Release();

private:
	void InitCommonMeshes();

private:
	// Mesh data
	std::unordered_map<std::string, NXMeshViews> m_data; 

	std::mutex m_mutex;
};
