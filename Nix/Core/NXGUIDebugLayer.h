#pragma once
#include "Header.h"

class NXDebugLayerRenderer;

class NXGUIDebugLayer
{
public:
	NXGUIDebugLayer(NXDebugLayerRenderer* pDebugLayer) : m_pDebugLayer(pDebugLayer) {}
	~NXGUIDebugLayer() {}

	void Render();

private:
	NXDebugLayerRenderer* m_pDebugLayer;
};