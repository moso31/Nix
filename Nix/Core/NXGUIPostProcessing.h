#pragma once
#include "Renderer.h"

class NXGUIPostProcessing
{
public:
	NXGUIPostProcessing(Renderer* pRenderer);
	virtual ~NXGUIPostProcessing() {}

	void Render();

private:
	Renderer* m_pRenderer;
};