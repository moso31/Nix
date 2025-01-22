#pragma once
#include <string>
#include "BaseDefs/DX12.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

struct NXMeshViews
{
	NXMeshViews(const std::string& name) : 
		name(name),
		loadFuture(loadPromise.get_future()), 
		loadCounter(0) {}

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

	std::string name;

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

	template<class TVertex>
	void CreateVBIB(const std::vector<TVertex>& vertices, std::vector<UINT>& indices, const std::string& name)
	{
		// ��д������Ҫ���������������߳�ͬʱ����ͬһ��mesh��VBIB
		// TODO���ĳɶ�д��
		{	
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_data.find(name) != m_data.end())
				return;

			NXMeshViews* pMeshView = new NXMeshViews(name);
			pMeshView->loadCounter = 2;
			m_data.emplace(name, pMeshView);
		}

		// �첽���������������ݺ�vbv/ibv�����ϴ�
		// TODO: Ŀǰ��������vertices indices ���ǳ�Ա�������������ﲻ��Ҫ���������������⣬����������˵����
		std::thread([&vertices, &indices, name, this]() {
			// �������߳�A�����ڴ�
			NXStructuredBuffer<TVertex> pVB(vertices.size());
			NXStructuredBuffer<UINT> pIB(indices.size());

			// ������ڴ��������һ���첽�߳�B���ȴ��߳�B�������
			pVB.WaitCreateComplete();
			pIB.WaitCreateComplete();

			UINT vbDataSize = (UINT)(vertices.size() * sizeof(TVertex));
			UINT ibDataSize = (UINT)(indices.size() * sizeof(UINT));

			NXMeshViews* views = m_data[name];
			views->vbv.BufferLocation = pVB.GetGPUAddress();
			views->vbv.SizeInBytes = vbDataSize;
			views->vbv.StrideInBytes = sizeof(TVertex);
			views->ibv.BufferLocation = pIB.GetGPUAddress();
			views->ibv.SizeInBytes = ibDataSize;
			views->ibv.Format = DXGI_FORMAT_R32_UINT;
			views->indexCount = (UINT)indices.size();
			views->vertexCount = (UINT)vertices.size();

			UploadTaskContext vbTaskContext(name + "_VB");
			UploadTaskContext ibTaskContext(name + "_IB");
			if (NXUploadSystem->BuildTask(vbDataSize, vbTaskContext))
			{
				// vbTaskContext.pResourceData/pResourceOffset�� �ϴ�ϵͳ��UploadRingBuffer ����ʱ��Դ��ƫ����
				// pVB.GetD3DResourceAndOffset(byteOffset) �� D3D���� ��ʵ����Դ��ƫ����
				// ��Ҫ�����

				uint8_t* pDst = vbTaskContext.pResourceData + vbTaskContext.pResourceOffset;
				memcpy(pDst, vertices.data(), vbDataSize);

				uint64_t byteOffset = 0;
				ID3D12Resource* pDstResource = pVB.GetD3DResourceAndOffset(byteOffset);
				vbTaskContext.pOwner->pCmdList->CopyBufferRegion(pDstResource, byteOffset, vbTaskContext.pResource, vbTaskContext.pResourceOffset, vbDataSize);

				// �ϴ����ݲ�ͬ����gpu�����ڲ���һ���첽�߳�C
				NXUploadSystem->FinishTask(vbTaskContext, [this, name, taskID = vbTaskContext.pOwner->selfID]() {
					m_data[name]->ProcessOne(); // ���������ϴ���ɣ�֪ͨ loadCounter - 1
					});
			}

			if (NXUploadSystem->BuildTask(ibDataSize, ibTaskContext))
			{
				uint8_t* pDst = ibTaskContext.pResourceData + ibTaskContext.pResourceOffset;
				memcpy(pDst, indices.data(), ibDataSize);

				uint64_t byteOffset = 0;
				ID3D12Resource* pDstResource = pIB.GetD3DResourceAndOffset(byteOffset);
				ibTaskContext.pOwner->pCmdList->CopyBufferRegion(pDstResource, byteOffset, ibTaskContext.pResource, ibTaskContext.pResourceOffset, ibDataSize);

				NXUploadSystem->FinishTask(ibTaskContext, [this, name, taskID = ibTaskContext.pOwner->selfID]() {
					m_data[name]->ProcessOne(); // ���������ϴ���ɣ�֪ͨ loadCounter - 1
					});
			}

			}).detach();
	}

	const NXMeshViews& GetMeshViews(const std::string& name);

	void Release();

private:
	void InitCommonMeshes();

private:
	// Mesh data
	std::unordered_map<std::string, NXMeshViews*> m_data; 

	std::mutex m_mutex;

	std::vector<float> m_verticesUnknown;
	std::vector<UINT> m_indicesUnknown;

	std::vector<VertexPT> m_verticesRT;
	std::vector<UINT> m_indicesRT;
};
