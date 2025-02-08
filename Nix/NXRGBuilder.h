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

	// ����һ��RenderGraphʹ�õ���Դ��
	NXRGResource* Create(const NXRGDescription& desc);

	// ����Pass������Դ��
	// passSlotIndex = ����pass shaderʹ�õ�slot������
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// ����pass���RT/DS��
	// outRTIndex = pass shader �����RT������DS���Դ˲�����
	NXRGResource* Write(NXRGResource* pResource, uint32_t outRTIndex = -1);

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
