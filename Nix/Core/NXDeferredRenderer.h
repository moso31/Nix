#pragma once
#include "BaseDefs/DX11.h"
#include "ShaderStructures.h"

class NXScene;
class NXBRDFLut;
class NXRenderTarget;
class NXDeferredRenderer
{
public:
	NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	~NXDeferredRenderer();

	void Init();
	void Render();

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
