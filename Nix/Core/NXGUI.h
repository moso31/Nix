#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/DearImGui.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "DirectResources.h"

class NXScene;
class Renderer;
class NXTexture2D;

class NXGUIFileBrowser;
class NXGUIContentExplorer;
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
	void Render(Ntr<NXTexture2D> pGUIViewRT, const NXSwapChainBuffer& swapChainBuffer);
	void Release();

private:
	NXShaderVisibleDescriptorHeap*	m_pImguiDescHeap;

	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	m_pCmdList;
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		m_pCmdAllocator;

	bool m_bInited = false;
	NXScene*	m_pCurrentScene;
	Renderer*	m_pRenderer;

	NXGUIContentExplorer*		m_pGUIContentExplorer;
	NXGUILights*				m_pGUILights;
	NXGUICamera*				m_pGUICamera;
	NXGUICubeMap*				m_pGUICubeMap;
	NXGUIFileBrowser*			m_pFileBrowser;
	NXGUISSAO*					m_pGUISSAO;
	NXGUIShadows*				m_pGUIShadows;
	NXGUIPostProcessing*		m_pGUIPostProcessing;
	NXGUIView*					m_pGUIView;
	NXGUIWorkspace*				m_pGUIWorkspace;
	NXGUIInspector*				m_pGUIInspector;
	NXGUIMaterialShaderEditor*	m_pGUIMaterialShaderEditor;

	NXGUIDebugLayer*			m_pGUIDebugLayer;
};
