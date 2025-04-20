#pragma once
#include "NXSubMeshCommon.h"
#include "NXAllocatorManager.h"
#include "ShaderStructures.h"
#include "NXInstance.h"
#include "NXStructuredBuffer.h"

class NXPrefab;
class NXPrimitive;
class NXTerrain;
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
	void CreateTerrain(NXTerrain* pTerrain, int rawSize, int gridSize, int worldSize, const std::filesystem::path& rawFile);

	// Editor objects
	void CreateMoveArrows(NXPrimitive* pMesh);

	void CreateBuffers(std::vector<NXRawMeshView>& rawViews, const std::string& name)
	{
		NXMeshViews* pMeshView = nullptr;
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			if (m_data.find(name) != m_data.end())
				return;

			pMeshView = new NXMeshViews(name, rawViews);
			m_data.emplace(name, pMeshView);
		}

		if (!pMeshView)
			return;

		// �첽����rawViews������/����/instanceData�����ݵ�vbv/ibv�����ϴ�
		// �ϲ�Ŀǰ����ȷ��rawViews ���ǳ�Ա�������������ﲻ��Ҫ���������������⣬����������˵����
		
		// �߳�A ִ�� std::thread
		printf("���߳�%s\n", name.c_str());
		std::thread([&rawViews, name, pMeshView]() { 
			for (auto& view : rawViews)
			{
				// �����첽�����߼������߳�Bִ��
				NXStructuredBuffer pBuffer(view.stride, view.span.size());
				// �ȴ��߳�B��ɷ��� get GPUAddress.
				pBuffer.WaitCreateComplete();

				view.gpuAddress = pBuffer.GetGPUAddress();

				uint32_t byteSize = view.span.size_bytes();
				UploadTaskContext ctx(name + "_VB");
				if (NXUploadSystem->BuildTask(byteSize, ctx))
				{
					// ctx.pResourceData/pResourceOffset�� �ϴ�ϵͳ��UploadRingBuffer ����ʱ��Դ��ƫ����
					// pVB.GetD3DResourceAndOffset(byteOffset) �� D3DĬ�϶��� ��ƫ������Ҳ����GPU��Դ
					// ��Ҫ�����

					uint8_t* pDst = ctx.pResourceData + ctx.pResourceOffset;

					// �������ϴ���
					memcpy(pDst, view.span.data(), byteSize);

					// ���ϴ��ѿ�����Ĭ�϶�
					uint64_t byteOffset = 0;
					ID3D12Resource* pDstResource = pBuffer.GetD3DResourceAndOffset(byteOffset);
					ctx.pOwner->pCmdList->CopyBufferRegion(pDstResource, byteOffset, ctx.pResource, ctx.pResourceOffset, byteSize);

					// �ϴ����ݲ�ͬ����gpu���첽�߳�C ����ִ��
					NXUploadSystem->FinishTask(ctx, [pMeshView, name, taskID = ctx.pOwner->selfID]() {
						pMeshView->ProcessOne(); // ���������ϴ���ɣ�֪ͨ loadCounter - 1
						});
				}
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

	NXSubMesh<float>* m_subMeshUnknown;
	NXSubMesh<VertexPT>* m_subMeshRT;
};
