#pragma once

class NXScene;
class NXPBRLight;
class NXGUITerrainSystem
{
public:
	NXGUITerrainSystem(NXScene* pScene = nullptr);
	virtual ~NXGUITerrainSystem() {}

	void Render();
	void Show() { m_bShowWindow = true; }

private:
	void Render_List();
	void Render_Map();
	void Render_Tools();

private:
	NXScene* m_pCurrentScene;
	bool m_bShowWindow;
};