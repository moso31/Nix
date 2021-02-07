#include "NXGUI.h"
#include "DirectResources.h"

NXGUI::NXGUI()
{
}

NXGUI::~NXGUI()
{
}

void NXGUI::Init()
{
	// UI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pDevice.Get(), g_pContext.Get());
}

void NXGUI::Render()
{
	g_pUDA->BeginEvent(L"GUI");

	// 以上操作全部都是在主RTV中进行的。
	// 下面切换到QuadRTV，简单来说就是将主RTV绘制到这个RTV，然后作为一张四边形纹理进行最终输出。
	auto pRenderTargetView = g_dxResources->GetRenderTargetView();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();
	g_pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);
	g_pContext->ClearRenderTargetView(pRenderTargetView, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->RSSetState(nullptr);
	g_pContext->OMSetDepthStencilState(nullptr, 0);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	//g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	//g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// clear SRV.
	ID3D11ShaderResourceView* const pNullSRV[16] = { nullptr };
	g_pContext->PSSetShaderResources(0, 16, pNullSRV);

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	NX::ThrowIfFailed(g_pSwapChain->Present1(1, 0, &parameters));

	g_pUDA->EndEvent();
}

void NXGUI::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
