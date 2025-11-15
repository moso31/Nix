#pragma once
#include "BaseDefs/NixCore.h"
#include "NXRGUtil.h"

class NXRenderGraph;
class NXRGPassNodeBase;
class NXRGBuilder
{
	NXRGBuilder() = delete;
	NXRGBuilder(const NXRGBuilder&) = delete;
	NXRGBuilder& operator=(const NXRGBuilder&) = delete;

public:
	NXRGBuilder(NXRenderGraph* pRenderGraph, NXRGPassNodeBase* pPassNode) :
		m_pRenderGraph(pRenderGraph), m_pPassNode(pPassNode) {}

	NXRGPassNodeBase* GetPassNode() { return m_pPassNode; }

	// 目前pass对资源读取的方式分为以下三种：Read、Write、ReadWrite
	// Read：表示这个pass只读取这个资源
	// Write：表示这个pass写入这个资源
	// - 如果是Create资源，创建一个Handle的副本
	// - 如果是Import资源，创建一个Handle，但底层资源指针指向同一资源
	NXRGHandle Read(NXRGHandle resID);
	NXRGHandle Write(NXRGHandle resID);
	NXRGHandle ReadWrite(NXRGHandle resID);

private:
	NXRenderGraph* m_pRenderGraph;
	NXRGPassNodeBase* m_pPassNode;
};
