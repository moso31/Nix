#pragma once
#include "BaseDefs/NixCore.h"

enum NXGUIInspectorEnum
{
	NXGUIInspector_Nothing,
	NXGUIInspector_Texture,
	NXGUIInspector_Material,
	NXGUIInspector_SubsurfaceProfile,
};

struct NXGUICommand;
class NXGUITexture;
class NXGUIMaterial;
class NXGUIMaterialShaderEditor;
class NXGUITerrainSystem;
class NXGUIDiffuseProfile;
class NXScene;
class NXGUIInspector
{
public:
	NXGUIInspector();
	virtual ~NXGUIInspector() {}

	void InitGUI(NXScene* pScene, NXGUIMaterialShaderEditor* pMaterialShaderEditor, NXGUITerrainSystem* pTerrainSystem);

	void DoCommand(const NXGUICommand& cmd);
	void Render();

	void Release();

private:
	void Render_Texture();
	void Render_Material();
	void Render_SubsurfaceProfiler();

private:
	NXGUIInspectorEnum m_inspectorIndex;
	NXGUITexture* m_pGUITexture;
	NXGUIMaterial* m_pGUIMaterial;
	NXGUIDiffuseProfile* m_pGUIDiffuseProfile;
	NXGUIMaterialShaderEditor* m_pGUIMaterialShaderEditor;
	NXGUITerrainSystem* m_pGUITerrainSystem;
};
