#pragma once
#include "BaseDefs/DearImGui.h"

class Renderer;
class NXGUITerrainSector2NodeIDPreview
{
public:
	NXGUITerrainSector2NodeIDPreview();
	virtual ~NXGUITerrainSector2NodeIDPreview();

	void Init(Renderer* pRenderer);
	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	Renderer* m_pRenderer = nullptr;
	bool m_bVisible = false;
	float m_zoomScale = 1.0f;
};
