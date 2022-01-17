#pragma once

class NXScene;
class NXSimpleSSAO;

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUISSAO;
class NXGUICubeMap;
class NXGUILights;

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

	NXGUILights* m_pGUILights;
	NXGUIMaterial* m_pGUIMaterial;
	NXGUICubeMap* m_pGUICubeMap;
	NXGUIFileBrowser* m_pFileBrowser;
	NXGUISSAO* m_pGUISSAO;
};
