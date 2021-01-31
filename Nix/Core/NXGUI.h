// dear imgui: Renderer Backend for DirectX11
// This needs to be used along with a Platform Backend (e.g. Win32)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'ID3D11ShaderResourceView*' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Support for large meshes (64k+ vertices) with 16-bit indices.

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#include "Header.h"

class NXGUI
{
public:
	NXGUI();
	~NXGUI();

	bool Init(ID3D11Device* device, ID3D11DeviceContext* device_context);
	void NewFrame();
	void RenderDrawData(ImDrawData* draw_data);
	void Shutdown();

	// Use if you want to reset your rendering device without losing Dear ImGui state.
	void InvalidateDeviceObjects();
	bool CreateDeviceObjects();

private:
	void _SetupRenderState(ImDrawData* draw_data, ID3D11DeviceContext* ctx);
	void _CreateFontsTexture();

private:
	// DirectX data
	ComPtr<ID3D11Device>				m_pd3dDevice;
	ComPtr<ID3D11DeviceContext>			m_pd3dDeviceContext;
	ComPtr<IDXGIFactory>				m_pFactory;
	ComPtr<ID3D11Buffer>				m_pVB;
	ComPtr<ID3D11Buffer>				m_pIB;
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;
	ComPtr<ID3D11Buffer>				m_pVertexConstantBuffer;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11SamplerState>			m_pFontSampler;
	ComPtr<ID3D11ShaderResourceView>	m_pFontTextureView;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;
	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	int m_VertexBufferSize, m_IndexBufferSize;

};
