#pragma once
#include "Renderer.h"

class NXGUIRenderGraph
{
public:
	NXGUIRenderGraph(Renderer* pRenderer);
	virtual ~NXGUIRenderGraph() {}

	void Render();

private:
	Renderer* m_pRenderer;
	Ntr<NXResource> m_pShowResource;
};
