#pragma once

class NXScene;
class Renderer;
class NXTexture2D;

class NXGUITexture;
class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUIContentExplorer;
class NXGUISSAO;
class NXGUICubeMap;
class NXGUILights;
class NXGUICamera;
class NXGUIShadows;
class NXGUIPostProcessing;
class NXGUIDebugLayer;
class NXGUIView;
class NXGUIWorkspace;

class NXGUI
{
public:
	NXGUI(NXScene* pScene, Renderer* pRenderer);
	~NXGUI();

	void Init();
	void Render(NXTexture2D* pGUIViewRT);
	void Release();

private:
	NXScene*				m_pCurrentScene;
	Renderer*				m_pRenderer;

	NXGUIContentExplorer*	m_pGUIContentExplorer;
	NXGUITexture*			m_pGUITexture;

	NXGUILights*			m_pGUILights;
	NXGUICamera*			m_pGUICamera;
	NXGUIMaterial*			m_pGUIMaterial;
	NXGUICubeMap*			m_pGUICubeMap;
	NXGUIFileBrowser*		m_pFileBrowser;
	NXGUISSAO*				m_pGUISSAO;
	NXGUIShadows*			m_pGUIShadows;
	NXGUIPostProcessing*	m_pGUIPostProcessing;
	NXGUIView*				m_pGUIView;
	NXGUIWorkspace*			m_pGUIWorkspace;

	NXGUIDebugLayer*		m_pGUIDebugLayer;
};
