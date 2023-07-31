#include "NXGUI.h"
#include "NXEvent.h"
#include "DirectResources.h"
#include "Renderer.h"
#include "NXScene.h"
#include "NXConverter.h"

#include "NXGUIFileBrowser.h"
#include "NXGUICodeEditor.h"
#include "NXGUIMaterialShaderEditor.h"
#include "NXGUIMaterial.h"
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

NXGUI::NXGUI(NXScene* pScene, Renderer* pRenderer) :
	m_pCurrentScene(pScene),
	m_pRenderer(pRenderer),
	m_pFileBrowser(nullptr),
	m_pGUICodeEditor(nullptr),
	m_pGUIMaterialShaderEditor(nullptr),
	m_pGUIMaterial(nullptr),
	m_pGUILights(nullptr),
	m_pGUICamera(nullptr),
	m_pGUICubeMap(nullptr),
	m_pGUISSAO(nullptr),
	m_pGUIShadows(nullptr),
	m_pGUIPostProcessing(nullptr),
	m_pGUIDebugLayer(nullptr),
	m_pGUIContentExplorer(nullptr),
	m_pGUITexture(nullptr),
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

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pDevice.Get(), g_pContext.Get());

	// 设置字体
	io.Fonts->AddFontFromFileTTF("./Resource/fonts/JetBrainsMono-Bold.ttf", 16);

	ImFontConfig configData;
	configData.GlyphMinAdvanceX = configData.GlyphMaxAdvanceX = 7.0f;
	auto pCodeEditorFont = io.Fonts->AddFontFromFileTTF("./Resource/fonts/JetBrainsMono-Bold.ttf", 16, &configData);

	ImGui_ImplDX11_CreateDeviceObjects();

	m_pFileBrowser = new NXGUIFileBrowser();
	m_pFileBrowser->SetTitle("File Browser");
	m_pFileBrowser->SetPwd("D:\\NixAssets");

	m_pGUICodeEditor = new NXGUICodeEditor(pCodeEditorFont);
	m_pGUIMaterialShaderEditor = new NXGUIMaterialShaderEditor();

	m_pGUITexture = new NXGUITexture();
	m_pGUIContentExplorer = new NXGUIContentExplorer(m_pCurrentScene, m_pGUITexture);

	m_pGUICamera = new NXGUICamera(m_pCurrentScene);
	m_pGUIMaterial = new NXGUIMaterial(m_pCurrentScene, m_pFileBrowser, m_pGUICodeEditor, m_pGUIMaterialShaderEditor);
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
}

void NXGUI::Render(NXTexture2D* pGUIViewRT)
{
	g_pUDA->BeginEvent(L"GUI");

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 参考 imgui_demo.cpp 的注释：
	// ImGui::DockSpace 绘制顺序必须尽可能的早，
	// 在 DockSpace 之前绘制的 UI 无法吸附到 DockSpace 上。
	// 所以这里写在所有 UI 最前面。
	m_pGUIWorkspace->Render();

	m_pGUIContentExplorer->Render();
	m_pGUITexture->Render();
	m_pGUICubeMap->Render();
	m_pGUIMaterial->Render();
	m_pGUILights->Render();
	m_pGUICamera->Render();
	m_pGUISSAO->Render();
	m_pGUIShadows->Render();
	m_pGUIPostProcessing->Render();
	m_pGUIDebugLayer->Render();

	if (m_pGUIView->GetViewRT() != pGUIViewRT)
		m_pGUIView->SetViewRT(pGUIViewRT);
	m_pGUIView->Render();

	static bool show_demo_window = true;
	static bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	m_pFileBrowser->Display();

	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	g_pUDA->EndEvent();
}

void NXGUI::Release()
{
	//ImGui::SaveIniSettingsToDisk(NXConvert::GetPathOfImguiIni().c_str());

	SafeDelete(m_pGUIView);
	SafeDelete(m_pGUIWorkspace);
	SafeDelete(m_pGUIMaterialShaderEditor);
	SafeDelete(m_pGUICodeEditor);
	SafeRelease(m_pGUIMaterial);
	SafeDelete(m_pGUILights);
	SafeDelete(m_pGUICamera);
	SafeDelete(m_pGUICubeMap);
	SafeDelete(m_pGUISSAO);
	SafeDelete(m_pGUIShadows);
	SafeDelete(m_pFileBrowser);
	SafeDelete(m_pGUIPostProcessing);
	SafeDelete(m_pGUIDebugLayer);
	SafeRelease(m_pGUITexture);
	SafeDelete(m_pGUIContentExplorer);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
