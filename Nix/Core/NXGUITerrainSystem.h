#pragma once
#include <future>

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

	int m_bake_progress = 1;
	int m_bake_progress_count = 1;
	std::future<void> m_bake_future;
};