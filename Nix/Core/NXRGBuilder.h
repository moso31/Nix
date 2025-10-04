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

	// 设置Pass输入资源。
	// passSlotIndex = 最终pass shader使用的slot索引。
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// 设置Pass的根参数布局
	void SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount);

	// 设置Pass输入的CB
	// rootIndex = root signature中CB所在的索引
	// slotIndex = shader中CB所在的slot索引
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer);

	// 设置pass输出RT/DS。
	// outRTIndex = pass shader 输出的RT索引（DS忽略此参数）
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool keep = false);
	NXRGResource* WriteDS(NXRGResource* pResource, bool keep = false);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool keep = false, uint32_t uavCounterIndex = -1);
	NXRGResource* SetIndirectArgs(NXRGResource* pResource);
	void WriteReadbackData(Ntr<NXReadbackData>& data);

	// 设置线程组数量
	void SetComputeThreadGroup(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetEntryNameVS(const std::wstring& entryName);
	void SetEntryNamePS(const std::wstring& entryName);
	void SetEntryNameCS(const std::wstring& entryName);

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
