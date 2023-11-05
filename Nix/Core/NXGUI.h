#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX11.h"
#include "BaseDefs/DearImGui.h"

class NXScene;
class Renderer;
class NXTexture2D;

class NXGUIMaterial;
class NXGUIFileBrowser;
class NXGUIContentExplorer;
class NXGUICodeEditor;
class NXGUIMaterialShaderEditor;
class NXGUISSAO;
class NXGUICubeMap;
class NXGUILights;
class NXGUICamera;
class NXGUIShadows;
class NXGUIPostProcessing;
class NXGUIDebugLayer;
class NXGUIView;
class NXGUIWorkspace;
class NXGUIInspector;

class NXGUI
{
public:
	NXGUI(NXScene* pScene, Renderer* pRenderer);
	~NXGUI();

	void Init();
	void ExecuteDeferredCommands();
	void Render(Ntr<NXTexture2D> pGUIViewRT);
	void Release();

private:
	bool m_bInited = false;
	NXScene*	m_pCurrentScene;
	Renderer*	m_pRenderer;

	NXGUICodeEditor*			m_pGUICodeEditor;
	NXGUIMaterialShaderEditor*	m_pGUIMaterialShaderEditor;
	NXGUIContentExplorer*		m_pGUIContentExplorer;

	NXGUILights*				m_pGUILights;
	NXGUICamera*				m_pGUICamera;
	NXGUIMaterial*				m_pGUIMaterial;
	NXGUICubeMap*				m_pGUICubeMap;
	NXGUIFileBrowser*			m_pFileBrowser;
	NXGUISSAO*					m_pGUISSAO;
	NXGUIShadows*				m_pGUIShadows;
	NXGUIPostProcessing*		m_pGUIPostProcessing;
	NXGUIView*					m_pGUIView;
	NXGUIWorkspace*				m_pGUIWorkspace;
	NXGUIInspector*				m_pGUIInspector;

	NXGUIDebugLayer*			m_pGUIDebugLayer;
};
