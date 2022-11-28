#pragma once

class NXScene;
class NXSceneManager;
class Renderer;

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUISSAO;
class NXGUICubeMap;
class NXGUILights;
class NXGUICamera;
class NXGUIShadows;
class NXGUIDebugLayer;

class NXGUI
{
public:
	NXGUI(NXScene* pScene, Renderer* pRenderer);
	~NXGUI();

	void Init();
	void Render();
	void Release();

private:
	NXScene*				m_pCurrentScene;
	Renderer*				m_pRenderer;

	NXGUILights*		m_pGUILights;
	NXGUICamera*		m_pGUICamera;
	NXGUIMaterial*		m_pGUIMaterial;
	NXGUICubeMap*		m_pGUICubeMap;
	NXGUIFileBrowser*	m_pFileBrowser;
	NXGUISSAO*			m_pGUISSAO;
	NXGUIShadows*		m_pGUIShadows;

	NXGUIDebugLayer*	m_pGUIDebugLayer;
};
