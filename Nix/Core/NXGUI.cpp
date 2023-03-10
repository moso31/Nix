#include "NXGUI.h"
#include "NXEvent.h"
#include "DirectResources.h"
#include "Renderer.h"

#include "NXGUIFileBrowser.h"
#include "NXGUIMaterial.h"
#include "NXGUILights.h"
#include "NXGUICamera.h"
#include "NXGUICubeMap.h"
#include "NXGUISSAO.h"
#include "NXGUIShadows.h"
#include "NXGUIDebugLayer.h"
#include "NXGUIPostProcessing.h"
#include "NXGUIContentExplorer.h"

NXGUI::NXGUI(NXScene* pScene, Renderer* pRenderer) :
	m_pCurrentScene(pScene),
	m_pRenderer(pRenderer),
	m_pFileBrowser(nullptr),
	m_pGUIMaterial(nullptr),
	m_pGUILights(nullptr),
	m_pGUICamera(nullptr),
	m_pGUICubeMap(nullptr),
	m_pGUISSAO(nullptr),
	m_pGUIShadows(nullptr),
	m_pGUIPostProcessing(nullptr),
	m_pGUIDebugLayer(nullptr),
	m_pGUIContentExplorer(nullptr)
{
}

NXGUI::~NXGUI()
{
}

void NXGUI::Init()
{
	m_pFileBrowser = new NXGUIFileBrowser();
	m_pFileBrowser->SetTitle("File Browser");
	m_pFileBrowser->SetPwd("D:\\NixAssets");

	m_pGUIContentExplorer = new NXGUIContentExplorer(m_pCurrentScene);

	m_pGUICamera = new NXGUICamera(m_pCurrentScene);
	m_pGUIMaterial = new NXGUIMaterial(m_pCurrentScene, m_pFileBrowser);
	m_pGUILights = new NXGUILights(m_pCurrentScene);
	m_pGUICubeMap = new NXGUICubeMap(m_pCurrentScene, m_pFileBrowser);

	m_pGUISSAO = new NXGUISSAO(m_pRenderer->GetSSAORenderer());
	m_pGUIShadows = new NXGUIShadows(m_pRenderer->GetShadowMapRenderer());
	m_pGUIPostProcessing = new NXGUIPostProcessing(m_pRenderer->GetColorMappingRenderer());
	m_pGUIDebugLayer = new NXGUIDebugLayer(m_pRenderer->GetDebugLayerRenderer());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pDevice.Get(), g_pContext.Get());
}

void NXGUI::Render()
{
	g_pUDA->BeginEvent(L"GUI");

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_pGUIContentExplorer->Render();
	m_pGUICubeMap->Render();
	m_pGUIMaterial->Render();
	m_pGUILights->Render();
	m_pGUICamera->Render();
	m_pGUISSAO->Render();
	m_pGUIShadows->Render();
	m_pGUIPostProcessing->Render();
	m_pGUIDebugLayer->Render();

	static bool show_demo_window = true;
	static bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	//// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	//{
	//	static float f = 0.0f;
	//	static int counter = 0;

	//	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	//	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	//	ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
	//	ImGui::Checkbox("Another Window", &show_another_window);

	//	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
	//	ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

	//	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
	//		counter++;
	//	ImGui::SameLine();
	//	ImGui::Text("counter = %d", counter);

	//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	//	ImGui::End();
	//}

	//// 3. Show another simple window.
	//if (show_another_window)
	//{
	//	ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	//	ImGui::Text("Hello from another window!");
	//	if (ImGui::Button("Close Me"))
	//		show_another_window = false;
	//	ImGui::End();
	//}

	m_pFileBrowser->Display();

	// Rendering
	ImGui::Render();
	//g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	//g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	g_pUDA->EndEvent();
}

void NXGUI::Release()
{
	SafeDelete(m_pGUIMaterial);
	SafeDelete(m_pGUILights);
	SafeDelete(m_pGUICamera);
	SafeDelete(m_pGUICubeMap);
	SafeDelete(m_pGUISSAO);
	SafeDelete(m_pGUIShadows);
	SafeDelete(m_pFileBrowser);
	SafeDelete(m_pGUIPostProcessing);
	SafeDelete(m_pGUIDebugLayer);
	SafeDelete(m_pGUIContentExplorer);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
