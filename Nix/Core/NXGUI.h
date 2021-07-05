#pragma once
#include "NXGUIMaterial.h"
#include "NXGUIFileBrowser.h"
#include "NXGUISSAO.h"

class NXGUI
{
public:
	NXGUI(NXScene* pScene, NXSimpleSSAO* pSSAO);
	~NXGUI();

	void Init();
	void Render();
	void Release();

private:
	NXScene* m_pCurrentScene;
	NXSimpleSSAO* m_pSSAO;

	NXGUIMaterial* m_pGUIMaterial;
	NXGUIFileBrowser* m_pFileBrowser;
	NXGUISSAO* m_pGUISSAO;
};
