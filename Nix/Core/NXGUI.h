#pragma once
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DearImGui.h"
#include "BaseDefs/Math.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "DirectResources.h"
#include "NXReadbackData.h"

class NXScene;
class Renderer;
class NXTexture2D;

class NXGUIFileBrowser;
class NXGUITerrainSystem;
class NXGUIContentExplorer;
class NXGUIMaterialShaderEditor;
class NXGUICubeMap;
class NXGUILights;
class NXGUICamera;
class NXGUIShadows;
class NXGUIPostProcessing;
class NXGUIDebugLayer;
class NXGUIView;
class NXGUIWorkspace;
class NXGUIInspector;
class NXGUIRenderGraph;
class NXGUIVirtualTexture;
class NXGUIHoudiniTerrainExporter;
class NXGUITerrainMaterialGenerator;
class NXGUITerrainSector2NodeIDPreview;

class NXGUI
{
public:
	NXGUI(NXScene* pScene, Renderer* pRenderer);
	virtual ~NXGUI();

	void Init();
	void ExecuteDeferredCommands();
	void Render(Ntr<NXTexture2D> pGUIViewRT, const NXSwapChainBuffer& swapChainBuffer);
	void Release();

	void SetVTReadbackData(Ntr<NXReadbackData>& vtReadbackData) { m_vtReadbackData = vtReadbackData; }
	const Ntr<NXReadbackData>& GetVTReadbackData() const { return m_vtReadbackData; }
	void SetVTReadbackDataSize(const Int2& val) { m_vtReadbackDataSize = val; }
	const Int2 GetVTReadbackDataSize() const { return m_vtReadbackDataSize; }

	NXGUIHoudiniTerrainExporter* GetGUIHoudiniTerrainExporter() const { return m_pGUIHoudiniTerrainExporter; }
	NXGUITerrainMaterialGenerator* GetGUITerrainMaterialGenerator() const { return m_pGUITerrainMaterialGenerator; }

	// 延迟分配：打开窗口时分配，关闭窗口时释放
	void OpenGUITerrainSector2NodeIDPreview();

private:
	void UpdateGUITerrainSector2NodeIDPreview(); // 内部更新，检查是否需要释放

private:
	MultiFrame<ComPtr<ID3D12GraphicsCommandList>>	m_pCmdList;
	MultiFrame<ComPtr<ID3D12CommandAllocator>>		m_pCmdAllocator;

	bool m_bInited = false;
	NXScene*	m_pCurrentScene;
	Renderer*	m_pRenderer;

	NXGUITerrainSystem* 		m_pGUITerrainSystem;
	NXGUIVirtualTexture* 		m_pGUIVirtualTexture;
	NXGUIContentExplorer*		m_pGUIContentExplorer;
	NXGUILights*				m_pGUILights;
	NXGUICamera*				m_pGUICamera;
	NXGUICubeMap*				m_pGUICubeMap;
	NXGUIFileBrowser*			m_pFileBrowser;
	NXGUIShadows*				m_pGUIShadows;
	NXGUIPostProcessing*		m_pGUIPostProcessing;
	NXGUIView*					m_pGUIView;
	NXGUIWorkspace*				m_pGUIWorkspace;
	NXGUIInspector*				m_pGUIInspector;
	NXGUIMaterialShaderEditor*	m_pGUIMaterialShaderEditor;
	NXGUIRenderGraph* 			m_pGUIRenderGraph;
	NXGUIHoudiniTerrainExporter* m_pGUIHoudiniTerrainExporter;
	NXGUITerrainMaterialGenerator* m_pGUITerrainMaterialGenerator;
	NXGUITerrainSector2NodeIDPreview* m_pGUITerrainSector2NodeIDPreview;

	NXGUIDebugLayer*			m_pGUIDebugLayer;

	Ntr<NXReadbackData> m_vtReadbackData;
	Int2 m_vtReadbackDataSize;
};
