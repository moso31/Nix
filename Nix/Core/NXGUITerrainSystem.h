#pragma once

class NXScene;
class NXTerrain;
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

	void GenerateFile_Tex2DArray_HeightMap();

private:
	NXScene* m_pCurrentScene;
	NXTerrain* m_pPickingTerrain;
	bool m_bShowWindow;
	bool m_bPickTerrainSelectionChanged;
};