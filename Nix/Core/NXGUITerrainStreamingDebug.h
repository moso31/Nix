#pragma once
#include "BaseDefs/DearImGui.h"

class Renderer;
class NXGUITerrainStreamingDebug
{
public:
	NXGUITerrainStreamingDebug(Renderer* pRenderer);
	virtual ~NXGUITerrainStreamingDebug();

	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	Renderer* m_pRenderer = nullptr;
	bool m_bVisible = false;
};
