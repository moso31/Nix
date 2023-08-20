#pragma once
#include "header.h"
#include "ShaderStructures.h"

class NXDeferredRenderer
{
public:
	NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXDeferredRenderer();

	void Init();
	void Render(bool bSSSEnable);

	void Release();

private:
	// 进行最终渲染时使用此组Shaders
	ComPtr<ID3D11VertexShader>			m_pVertexShader; 
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	ComPtr<ID3D11DepthStencilState>		m_pDepthStencilState;
	ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	ComPtr<ID3D11BlendState>			m_pBlendState;

	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;

	NXRenderTarget* m_pResultRT;
};
