#pragma once
#include <future>
#include <filesystem>

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
	void BuildDockLayout(ImGuiID dockspace_id);
	void Render_List();
	void Render_Map();
	void Render_Tools();
	void ProcessAsyncCallback();

	void GenerateFile_Tex2DArray_HeightMap();

private:
	NXScene* m_pCurrentScene;
	NXTerrain* m_pPickingTerrain;
	bool m_bShowWindow;
	bool m_bPickTerrainSelectionChanged;
	float m_debugFrustumFactor = 0.0f;

	int m_bake_progress = 1;
	int m_bake_progress_count = 1;
	std::future<void> m_bake_future;
	bool m_bNeedUpdateTerrainLayerFiles = false;
};