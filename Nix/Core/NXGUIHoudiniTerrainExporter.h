#pragma once
#include "BaseDefs/DearImGui.h"

class NXGUIHoudiniTerrainExporter
{
public:
	NXGUIHoudiniTerrainExporter();
	virtual ~NXGUIHoudiniTerrainExporter();

	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	bool m_bVisible = false;
};
