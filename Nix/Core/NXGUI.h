#pragma once
#include "Header.h"
#include "NXScene.h"

class NXGUI
{
public:
	NXGUI();
	~NXGUI();

	void Init();
	void Render();
	void Release();

	void SetCurrentPrimitive(NXScene* pScene) { m_pCurrentScene = pScene; }

private:
	void RenderMaterial();
	void RenderTextureIcon(ImTextureID ImTexID);

private:
	NXScene* m_pCurrentScene;
};
