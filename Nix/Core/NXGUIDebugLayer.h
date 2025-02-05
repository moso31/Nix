#pragma once
#include "Renderer.h"

class NXGUIDebugLayer
{
public:
	NXGUIDebugLayer(Renderer* pRenderer) : m_pRenderer(pRenderer) {}
	virtual ~NXGUIDebugLayer() {}

	void Render();

private:
	Renderer* m_pRenderer;
};