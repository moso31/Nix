#pragma once
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

class NXScene;
class NXMaterial;
class NXCustomMaterial;
class NXGUICodeEditor;
struct NXGUIAssetDragData;
class NXTerrain;
class NXTerrainLayer;
class NXRenderableObject;
class NXGUIMaterial
{
public:
	NXGUIMaterial(NXScene* pScene);
	virtual ~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();
	void Release();

	void RequestSyncMaterialData() { m_bIsDirty = true; }

private:
	void RenderGUI_Unique_RenderableObject(NXRenderableObject* pObj);
	void RenderGUI_Unique_Terrain(NXTerrain* pTerrain);
	void RenderGUI_Unique_TerrainLayer(NXTerrain* pTerrain, NXTerrainLayer* pTerrainLayer);
	void RenderGUI_Unique_TerrainLayer_Connection(NXTerrain* pTerrain, NXTerrainLayer* pTerrainLayer);
	void RenderMaterialUI_Custom(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters(NXCustomMaterial* pMaterial);
	void RenderMaterialUI_Custom_Parameters_CBufferItem(const std::string& strId, NXCustomMaterial* pMaterial, NXMatDataCBuffer* cbData);

private:
	void OnBtnEditShaderClicked(NXCustomMaterial* pMaterial);

	void SyncMaterialData(NXCustomMaterial* pMaterial);

private:
	NXScene* m_pCurrentScene;
	NXMaterial* m_pLastCommonPickMaterial = nullptr;

	NXMaterialData m_guiData;

	NXCustomMaterial* m_pLastMaterial = nullptr;
	bool m_bIsDirty;
};
