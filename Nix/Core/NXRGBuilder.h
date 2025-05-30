#pragma once
#include "BaseDefs/NixCore.h"

class NXRenderGraph;
class NXRGResource;
class NXRGPassNodeBase;
struct NXRGDescription;
class NXRGBuilder
{
	NXRGBuilder() = delete;
	NXRGBuilder(const NXRGBuilder&) = delete;
	NXRGBuilder& operator=(const NXRGBuilder&) = delete;

public:
	NXRGBuilder(NXRenderGraph* pRenderGraph, NXRGPassNodeBase* pPassNode) :
		m_pRenderGraph(pRenderGraph), m_pPassNode(pPassNode) {}

	// ����Pass������Դ��
	// passSlotIndex = ����pass shaderʹ�õ�slot������
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// ����pass���RT/DS��
	// outRTIndex = pass shader �����RT������DS���Դ˲�����
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep = false);
	NXRGResource* WriteDS(NXRGResource* pResource, bool keep = false);

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
