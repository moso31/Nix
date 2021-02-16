#include "NXGUI.h"
#include "DirectResources.h"

NXGUI::NXGUI() :
	m_pCurrentScene(nullptr)
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

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	RenderMaterial();

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

	// Rendering
	ImGui::Render();
	//g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	//g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	g_pUDA->EndEvent();
}

void NXGUI::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void NXGUI::RenderMaterial()
{
	NXPrimitive* pPickingObject = static_cast<NXPrimitive*>(m_pCurrentScene->GetCurrentPickingObject());
	if (!pPickingObject)
		return;

	ImGui::Begin("Material");

	//ImGui::Text("Name");
	//ImGui::SameLine();
	//ImGui::InputText("Name"pPickingObject->GetName().c_str());

	std::string strName = pPickingObject->GetName().c_str();
	ImGui::InputText("Name", &strName);

	XMVECTOR value = XMLoadFloat3(&pPickingObject->GetTranslation());
	ImGui::DragFloat3("Translation", value.m128_f32);

	value = XMLoadFloat3(&pPickingObject->GetRotation().EulerXYZ());
	ImGui::DragFloat3("Rotation", value.m128_f32);

	value = XMLoadFloat3(&pPickingObject->GetScale());
	ImGui::DragFloat3("Scale", value.m128_f32);

	NXPBRMaterial* pPickingObjectMaterial = pPickingObject->GetPBRMaterial();
	if (pPickingObjectMaterial)
	{
		static XMVECTOR value = { 1.0, 1.0, 1.0 };

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAlbedo());
		ImGui::SameLine();
		ImGui::ColorEdit3("Albedo", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVNormal());
		ImGui::SameLine();
		ImGui::ColorEdit3("Normal", value.m128_f32);

		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVMetallic());
		ImGui::SameLine();
		ImGui::DragFloat("Metallic", value.m128_f32);
		
		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVRoughness());
		ImGui::SameLine();
		ImGui::DragFloat("Roughness", value.m128_f32);
		
		RenderTextureIcon((ImTextureID)pPickingObjectMaterial->GetSRVAO());
		ImGui::SameLine();
		ImGui::DragFloat("AO", value.m128_f32);
	}

	ImGui::End();
}

void NXGUI::RenderTextureIcon(ImTextureID ImTexID)
{
	float my_tex_w = (float)16;
	float my_tex_h = (float)16;

	ImGuiIO& io = ImGui::GetIO();
	{
		//ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
		ImGui::Image(ImTexID, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			float region_sz = 32.0f;
			float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
			float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
			float zoom = 4.0f;
			if (region_x < 0.0f) { region_x = 0.0f; }
			else if (region_x > my_tex_w - region_sz) { region_x = my_tex_w - region_sz; }
			if (region_y < 0.0f) { region_y = 0.0f; }
			else if (region_y > my_tex_h - region_sz) { region_y = my_tex_h - region_sz; }
			ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
			ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
			ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
			ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
			ImGui::Image(ImTexID, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
			ImGui::EndTooltip();
		}
	}
}
