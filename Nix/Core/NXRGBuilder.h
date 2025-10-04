#pragma once
#include "BaseDefs/NixCore.h"

class NXRenderGraph;
class NXRGResource;
class NXRGPassNodeBase;
class NXReadbackData;
struct NXRGDescription;
class NXConstantBufferImpl;
class NXRGBuilder
{
	NXRGBuilder() = delete;
	NXRGBuilder(const NXRGBuilder&) = delete;
	NXRGBuilder& operator=(const NXRGBuilder&) = delete;

public:
	NXRGBuilder(NXRenderGraph* pRenderGraph, NXRGPassNodeBase* pPassNode) :
		m_pRenderGraph(pRenderGraph), m_pPassNode(pPassNode) {}

	void SetSubmitGroup(uint32_t index);

	// ����Pass������Դ��
	// passSlotIndex = ����pass shaderʹ�õ�slot������
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// ����Pass�ĸ���������
	void SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount);

	// ����Pass�����CB
	// rootIndex = root signature��CB���ڵ�����
	// slotIndex = shader��CB���ڵ�slot����
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer);

	// ����pass���RT/DS��
	// outRTIndex = pass shader �����RT������DS���Դ˲�����
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep = false);
	NXRGResource* WriteDS(NXRGResource* pResource, bool keep = false);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool keep = false, uint32_t uavCounterIndex = -1);
	NXRGResource* SetIndirectArgs(NXRGResource* pResource);
	void WriteReadbackData(Ntr<NXReadbackData>& data);

	// �����߳�������
	void SetComputeThreadGroup(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetEntryNameVS(const std::wstring& entryName);
	void SetEntryNamePS(const std::wstring& entryName);
	void SetEntryNameCS(const std::wstring& entryName);

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
