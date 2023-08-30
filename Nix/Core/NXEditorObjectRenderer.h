#pragma once
#include "ShaderStructures.h"
#include "Ntr.h"

class NXTexture2D;
class NXColorMappingRenderer;
class NXEditorObjectRenderer
{
	struct CBufferParams_Internal
	{
		Vector4 params; // x : selected. 
	};

public:
	NXEditorObjectRenderer(NXScene* pScene);
	~NXEditorObjectRenderer();

	void Init();
	void OnResize(const Vector2& rtSize);
	void Render();

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	ComPtr<ID3D11Buffer>				m_cbParams;
	CBufferParams_Internal				m_cbDataParams;

	NXScene*							m_pScene;
	NXRenderTarget*						m_pRTQuad;
	
	// pass output resources
	Ntr<NXTexture2D>					m_pPassOutTex;

	bool m_bEnable;
};
