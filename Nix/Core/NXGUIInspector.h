#pragma once
#include "BaseDefs/NixCore.h"

enum NXGUIInspectorEnum
{
	NXGUIInspector_Nothing,
	NXGUIInspector_Texture,
	NXGUIInspector_Material,
	NXGUIInspector_SubsurfaceProfiler,
};

struct NXGUICommand;
class NXGUITexture;
class NXGUIMaterial;
class NXGUIMaterialShaderEditor;
class NXScene;
class NXGUIInspector
{
public:
	NXGUIInspector();
	~NXGUIInspector() {}

	void InitGUI(NXScene* pScene, NXGUIMaterialShaderEditor* pMaterialShaderEditor);

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
	NXGUIMaterialShaderEditor* m_pGUIMaterialShaderEditor;
};
