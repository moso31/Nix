#pragma once
#include "NXDebugLayerRenderer.h"

class NXDebugLayerRenderer;

class NXGUIDebugLayer
{
public:
	NXGUIDebugLayer(NXDebugLayerRenderer* pDebugLayer) : m_pDebugLayer(pDebugLayer) {}
	virtual ~NXGUIDebugLayer() {}

	void Render();

private:
	NXDebugLayerRenderer* m_pDebugLayer;
};