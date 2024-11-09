#pragma once
#include <string>
#include "BaseDefs/DX12.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

struct NXMeshViews
{
	NXMeshViews() : loadFuture(loadPromise.get_future()), loadCounter(0) {}

	void ProcessOne()
	{
		if (--loadCounter == 0)
		{
			loadPromise.set_value();
		}
	}

	void WaitLoadComplete()
	{
		loadFuture.wait();
	}

	std::atomic<int> loadCounter;
	std::promise<void> loadPromise;
	std::future<void> loadFuture;

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

	// 2024.10.15 
	// 因为这里需要确保顶点/索引生命周期完整覆盖多线程，
	// 所以约定上层接口调用CreateVBIB时，必须放弃std::vector<>的生命周期控制权。（所以用了右值引用） 
	template<class TVertex>
	void CreateVBIB(std::vector<TVertex>&& vertices, std::vector<UINT>&& indices, const std::string& name, bool forceCreate = false)
	{
		if (m_data.find(name) != m_data.end())
		{
			if (!forceCreate)
			{
				//throw std::exception("Mesh name already exists.");
				return;
			}
		}

		std::thread([vertices = std::move(vertices), indices = std::move(indices), name, this]() {
			NXStructuredBuffer<TVertex> pVB(vertices.size());
			NXStructuredBuffer<UINT> pIB(indices.size());

			pVB.WaitCreateComplete();
			pIB.WaitCreateComplete();

			UINT vbDataSize = (UINT)(vertices.size() * sizeof(TVertex));
			UINT ibDataSize = (UINT)(indices.size() * sizeof(UINT));

			NXMeshViews views;
			views.vbv.BufferLocation = pVB.CurrentGPUAddress();
			views.vbv.SizeInBytes = vbDataSize;
			views.vbv.StrideInBytes = sizeof(TVertex);
			views.ibv.BufferLocation = pIB.CurrentGPUAddress();
			views.ibv.SizeInBytes = ibDataSize;
			views.ibv.Format = DXGI_FORMAT_R32_UINT;
			views.indexCount = (UINT)indices.size();
			views.vertexCount = (UINT)vertices.size();
			views.loadCounter = 2; // vb + ib.

			{ 
				std::lock_guard<std::mutex> lock(m_mutex);
				m_data[name] = std::move(views);
			}

			UploadTaskContext vbTaskContext;
			UploadTaskContext ibTaskContext;
			if (NXUploadSystem->BuildTask(vbDataSize, vbTaskContext))
			{
				uint8_t* pDst = vbTaskContext.pResourceData + vbTaskContext.pResourceOffset;
				memcpy(pDst, vertices.data(), vbDataSize);
				NXUploadSystem->FinishTask(vbTaskContext, [this, name]() { m_data[name].ProcessOne(); });
			}

			if (NXUploadSystem->BuildTask(ibDataSize, ibTaskContext))
			{
				uint8_t* pDst = ibTaskContext.pResourceData + ibTaskContext.pResourceOffset;
				memcpy(pDst, indices.data(), ibDataSize);
				NXUploadSystem->FinishTask(ibTaskContext, [this, name]() { m_data[name].ProcessOne(); });
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
