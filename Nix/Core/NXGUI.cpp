#include "NXGlobalDefinitions.H"

#include "NXGUI.h"
#include "NXEvent.h"
#include "DirectResources.h"
#include "Renderer.h"
#include "NXScene.h"
#include "NXConverter.h"
#include "NXAllocatorManager.h"

#include "NXGUIFileBrowser.h"
#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIInspector.h"
#include "NXGUILights.h"
#include "NXGUICamera.h"
#include "NXGUICubeMap.h"
#include "NXGUISSAO.h"
#include "NXGUIShadows.h"
#include "NXGUIDebugLayer.h"
#include "NXGUIPostProcessing.h"
#include "NXGUIContentExplorer.h"
#include "NXGUITexture.h"
#include "NXGUIView.h"
#include "NXGUIWorkspace.h"
#include "NXGUICommandManager.h"

NXGUI::NXGUI(NXScene* pScene, Renderer* pRenderer) :
	m_pCurrentScene(pScene),
	m_pRenderer(pRenderer),
	m_pFileBrowser(nullptr),
	m_pGUIMaterialShaderEditor(nullptr),
	m_pGUILights(nullptr),
	m_pGUICamera(nullptr),
	m_pGUICubeMap(nullptr),
	m_pGUISSAO(nullptr),
	m_pGUIShadows(nullptr),
	m_pGUIPostProcessing(nullptr),
	m_pGUIDebugLayer(nullptr),
	m_pGUIContentExplorer(nullptr),
	m_pGUIView(nullptr),
	m_pGUIWorkspace(nullptr)
{
}

NXGUI::~NXGUI()
{
}

void NXGUI::Init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		m_pCmdAllocator[i] = NX12Util::CreateCommandAllocator(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_pCmdList[i] = NX12Util::CreateGraphicsCommandList(NXGlobalDX::GetDevice(), m_pCmdAllocator.Get(i).Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	m_pImguiDescHeap = new NXShaderVisibleDescriptorHeap(NXGlobalDX::GetDevice());
	m_pImguiDescHeap->GetHeap()->SetName(L"ImGui shader visible heap");
	ImGui_ImplDX12_Init(NXGlobalDX::GetDevice(), MultiFrameSets_swapChainCount, DXGI_FORMAT_R8G8B8A8_UNORM, m_pImguiDescHeap->GetHeap(), m_pImguiDescHeap->GetCPUHandle(0), m_pImguiDescHeap->GetGPUHandle(0));

	// ��������

	ImGui_ImplWin32_Init(NXGlobalWindows::hWnd);
	g_imgui_font_general = io.Fonts->AddFontFromFileTTF("./Resource/fonts/JetBrainsMono-Bold.ttf", 16);

	// CodeEditor ��Ҫʹ�ö������������á�
	// ��ֱ��ʹ�û������壬�����ǵȿ����壬����Ҳ��������
	ImFontConfig configData;
	configData.GlyphMinAdvanceX = configData.GlyphMaxAdvanceX = 7.0f;
	g_imgui_font_codeEditor = io.Fonts->AddFontFromFileTTF("./Resource/fonts/JetBrainsMono-Bold.ttf", 16, &configData);

	ImGui_ImplDX12_CreateDeviceObjects();

	m_pFileBrowser = new NXGUIFileBrowser();
	m_pFileBrowser->SetTitle("File Browser");
	m_pFileBrowser->SetPwd("D:\\NixAssets");

	m_pGUIMaterialShaderEditor = new NXGUIMaterialShaderEditor();
	m_pGUIContentExplorer = new NXGUIContentExplorer(m_pCurrentScene);

	m_pGUIInspector = new NXGUIInspector();
	m_pGUIInspector->InitGUI(m_pCurrentScene, m_pGUIMaterialShaderEditor);

	m_pGUICamera = new NXGUICamera(m_pCurrentScene);
	m_pGUILights = new NXGUILights(m_pCurrentScene);
	m_pGUICubeMap = new NXGUICubeMap(m_pCurrentScene, m_pFileBrowser);

	m_pGUISSAO = new NXGUISSAO(m_pRenderer->GetSSAORenderer());
	m_pGUIShadows = new NXGUIShadows(m_pRenderer->GetShadowMapRenderer());
	m_pGUIPostProcessing = new NXGUIPostProcessing(m_pRenderer->GetColorMappingRenderer());
	m_pGUIDebugLayer = new NXGUIDebugLayer(m_pRenderer->GetDebugLayerRenderer());

	m_pGUIView = new NXGUIView();

	m_pGUIWorkspace = new NXGUIWorkspace();
	m_pGUIWorkspace->Init();

	//ImGui::LoadIniSettingsFromDisk(NXConvert::GetPathOfImguiIni().c_str());

	NXGUICommandManager::GetInstance()->Init(m_pGUIInspector);

	m_bInited = true;
}

void NXGUI::ExecuteDeferredCommands()
{
	NXGUICommandManager::GetInstance()->Update();
}

void NXGUI::Render(Ntr<NXTexture2D> pGUIViewRT, const NXSwapChainBuffer& swapChainBuffer)
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// �ο� imgui_demo.cpp ��ע�ͣ�
	// ImGui::DockSpace ����˳����뾡���ܵ��磬
	// �� DockSpace ֮ǰ���Ƶ� UI �޷������� DockSpace �ϡ�
	// ��������д������ UI ��ǰ�档
	m_pGUIWorkspace->Render();

	m_pGUIMaterialShaderEditor->Render();
	m_pGUIContentExplorer->Render();
	m_pGUIInspector->Render();
	m_pGUICubeMap->Render();
	m_pGUILights->Render();
	m_pGUICamera->Render();
	m_pGUISSAO->Render();
	m_pGUIShadows->Render();
	m_pGUIPostProcessing->Render();
	m_pGUIDebugLayer->Render();

	//if (m_pGUIView->GetViewRT() != pGUIViewRT)
	//	m_pGUIView->SetViewRT(pGUIViewRT);
	//m_pGUIView->Render(m_pImguiDescHeap);

	static bool show_demo_window = true;
	static bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	m_pFileBrowser->Display();

	// Rendering
	ImGui::Render();

	auto pCmdAllocator = m_pCmdAllocator.Current().Get();
	auto pCmdList = m_pCmdList.Current().Get();

	pCmdAllocator->Reset();
	pCmdList->Reset(pCmdAllocator, nullptr);

	NX12Util::BeginEvent(pCmdList, "dear-imgui");

	auto pDescHeap = m_pImguiDescHeap->GetHeap();
	pCmdList->SetDescriptorHeaps(1, &pDescHeap);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainBuffer.pBuffer.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	pCmdList->ResourceBarrier(1, &barrier);

	// Render Dear ImGui graphics
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	pCmdList->ClearRenderTargetView(swapChainBuffer.rtvHandle, clear_color_with_alpha, 0, nullptr);
	pCmdList->OMSetRenderTargets(1, &swapChainBuffer.rtvHandle, FALSE, nullptr);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	pCmdList->ResourceBarrier(1, &barrier);

	NX12Util::EndEvent(pCmdList);

	pCmdList->Close();
	ID3D12CommandList* ppCmdLists[] = { pCmdList };
	NXGlobalDX::GetCmdQueue()->ExecuteCommandLists(1, ppCmdLists);

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void NXGUI::Release()
{
	//ImGui::SaveIniSettingsToDisk(NXConvert::GetPathOfImguiIni().c_str());

	SafeDelete(m_pGUIView);
	SafeDelete(m_pGUIWorkspace);
	SafeRelease(m_pGUIMaterialShaderEditor);
	SafeRelease(m_pGUIInspector);
	SafeDelete(m_pGUILights);
	SafeDelete(m_pGUICamera);
	SafeDelete(m_pGUICubeMap);
	SafeDelete(m_pGUISSAO);
	SafeDelete(m_pGUIShadows);
	SafeDelete(m_pFileBrowser);
	SafeDelete(m_pGUIPostProcessing);
	SafeDelete(m_pGUIDebugLayer);
	SafeDelete(m_pGUIContentExplorer);

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
