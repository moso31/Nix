#pragma once
#include "NXGUIMaterial.h"

class NXGUI
{
public:
	NXGUI();
	~NXGUI();

	void Init(NXScene* pScene);
	void Render();
	void Release();

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }

private:
	void RenderMaterial();
	void RenderTextureIcon(ImTextureID ImTexID);

private:
	NXScene* m_pCurrentScene;
	NXGUIMaterial* m_pMaterialGUI;
};
