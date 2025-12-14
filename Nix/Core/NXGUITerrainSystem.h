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
	// NXGUITerrainSystem 依赖 docking 功能实现布局；所以需要强制锁定
	void BuildDockLayout(ImGuiID dockspace_id); 

	// 左 地形列表：遍历场景所有地形并保存
	void Render_List();

	// 右上 HeightMap 预览
	void Render_Map();

	// 右下 工具按钮
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

	// 烘焙配置选项
	bool m_bakeConfig_GenerateHeightMap = true;
	bool m_bakeConfig_GenerateSplatMap = true;
	bool m_bakeConfig_ForceGenerate = false;
};