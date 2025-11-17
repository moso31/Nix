#pragma once
#include "Renderer.h"

enum class NXRGGUISortMode
{
	ByName,
	ByStartTime,
	ByDuration,
	None
};

enum class NXRGGUIResourceViewMode
{
	Virtual,
	Physical
};

class NXGUIRenderGraph
{
public:
	NXGUIRenderGraph(Renderer* pRenderer);
	virtual ~NXGUIRenderGraph() {}

	void Render();

private:
	Renderer* m_pRenderer;
	Ntr<NXResource> m_pShowResource;

	NXRGGUISortMode m_sortMode = NXRGGUISortMode::ByName;
	NXRGGUIResourceViewMode m_viewMode = NXRGGUIResourceViewMode::Virtual;
	bool m_showImportedResources = true;
};
