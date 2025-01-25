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

	// 声明一个RenderGraph使用的资源。
	NXRGResource* Create(const std::string& resourceName, const NXRGDescription& desc);

	// 设置Pass输入资源。
	// passSlotIndex = 最终pass shader使用的slot索引。
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// 设置pass输出RT/DS。
	// outRTIndex = pass shader 输出的RT索引（DS忽略此参数）
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep = false);
	NXRGResource* WriteDS(NXRGResource* pResource, bool keep = false);

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
