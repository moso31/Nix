#pragma once
#include "NXGUIMaterial.h"
#include "NXGUIFileBrowser.h"

class NXGUI
{
public:
	NXGUI(NXScene* pScene = nullptr);
	~NXGUI();

	void Init();
	void Render();
	void Release();

private:
	NXScene* m_pCurrentScene;

	NXGUIMaterial* m_pGUIMaterial;
	NXGUIFileBrowser* m_pFileBrowser;
};
