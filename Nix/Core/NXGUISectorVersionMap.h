#pragma once
#include "BaseDefs/DearImGui.h"

class Renderer;
class NXGUISectorVersionMap
{
public:
	NXGUISectorVersionMap(Renderer* pRenderer);
	virtual ~NXGUISectorVersionMap();

	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	Renderer* m_pRenderer = nullptr;
	bool m_bVisible = false;
	float m_cellSize = 40.0f;
};
